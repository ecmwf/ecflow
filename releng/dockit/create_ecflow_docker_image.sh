#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# create_ecflow_docker_image.sh
#
# Creates an ecflow Docker image (by default, ecflow-server-dev) from the ecflow
# Debian package(s) produced by build_ecflow_package.sh, i.e. <package>-<arch>.deb
# in the image build context (ecflow-server/, by default), for one or more platforms.
#
# Optionally, each platform image is smoke-tested before the image is loaded
# into the local Docker or pushed to a registry: it must not exceed a size limit
# (which catches, for example, a build carrying debug information), and must
# pass the server lifecycle, persistence and ownership checks. The 'dockerize' job of
# ecflow/.github/workflows/dockit.yml runs this script, so that both create the
# image in exactly the same way.
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CONTEXT="${SCRIPT_DIR}/ecflow-server"
PLATFORMS=""
TAGS=()
LABELS=()
BUILD_ARGS=()
SMOKE_TEST="false"
SMOKE_TEST_SCRIPT="${SCRIPT_DIR}/test_ecflow_server_image.sh"
MAX_SIZE_MB=400
PUSH="false"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function usage() {
    cat <<EOF
Usage: create_ecflow_docker_image.sh [options]

Creates an ecflow Docker image from the <package>-<arch>.deb package(s)
in the image build context, optionally smoke-tests it, and loads it into the
local Docker (default) or pushes it to a registry.

Options:
  --context DIR            Image build context, holding the Dockerfile and the
                             <package>-<arch>.deb of each platform
                             (default: ${CONTEXT})
  --platform LIST          Comma-separated target platforms, e.g.
                             linux/amd64,linux/arm64
                             (default: the platform of the Docker host)
  --tag NAME               Image name and tag; repeatable
                             (default: ecflow-server-dev:local)
  --label KEY=VALUE        Image label; repeatable
  --build-arg KEY=VALUE    Build argument, e.g. ECFLOW_UID=\$(id -u); repeatable
  --smoke-test             Before loading/pushing, build, start and check the
                             image of each platform (size, and the checks of the
                             smoke test script)
  --smoke-test-script FILE Script checking the image of a platform, run with
                             the image and the platform as arguments
                             (default: test_ecflow_server_image.sh, the server
                             lifecycle)
  --max-size-mb N          Size limit of the smoke test, in MB
                             (default: ${MAX_SIZE_MB})
  --push                   Push the image to its registry, instead of loading it
                             into the local Docker
  -h, --help               Show this help message and exit

Loading an image of several platforms into the local Docker requires its
containerd image store (the default of Docker Desktop).
EOF
}

# Smoke-tests the image of one platform: builds it, loads it into the local Docker, checks its size, and
# runs the smoke test script (by default, the server lifecycle and persistence checks) on it
function smoke_test() (
    local platform="$1"
    local scratch tag
    scratch=$(mktemp -d "${TMPDIR:-/tmp}/ecflow-smoke.XXXXXXXXXX")
    tag="ecflow-smoke:${scratch##*/}"

    # Reserve a unique tag for this invocation and clean up only that tag. A subshell keeps
    # these traps scoped to the smoke test, including failed builds and interrupted tests.
    trap 'docker image rm "${tag}" >/dev/null 2>&1 || true; rmdir "${scratch}"' EXIT
    trap 'exit 130' INT
    trap 'exit 143' TERM

    docker buildx build --platform "${platform}" --load -t "${tag}" \
        ${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"} "${CONTEXT}"

    local size size_mb
    size=$(docker image inspect -f '{{.Size}}' "${tag}")
    size_mb=$(( size / 1000000 ))
    echo "${platform}: image size ${size_mb} MB (limit ${MAX_SIZE_MB} MB)"
    if [[ "${size_mb}" -gt "${MAX_SIZE_MB}" ]]; then
        echo "${platform}: image exceeds ${MAX_SIZE_MB} MB" >&2
        return 1
    fi

    bash "${SMOKE_TEST_SCRIPT}" "${tag}" "${platform}"
)

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --context) CONTEXT="$2"; shift 2 ;;
        --platform) PLATFORMS="$2"; shift 2 ;;
        --tag) TAGS+=(--tag "$2"); shift 2 ;;
        --label) LABELS+=(--label "$2"); shift 2 ;;
        --build-arg) BUILD_ARGS+=(--build-arg "$2"); shift 2 ;;
        --smoke-test) SMOKE_TEST="true"; shift ;;
        --smoke-test-script) SMOKE_TEST_SCRIPT="$2"; shift 2 ;;
        --max-size-mb) MAX_SIZE_MB="$2"; shift 2 ;;
        --push) PUSH="true"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required to create the image, but was not found in PATH." >&2
    exit 1
fi

CONTEXT="$(cd "${CONTEXT}" && pwd)"

if [[ "${SMOKE_TEST}" == "true" ]] && [[ ! -f "${SMOKE_TEST_SCRIPT}" ]]; then
    echo "Smoke test script not found: ${SMOKE_TEST_SCRIPT}" >&2
    exit 1
fi

if [[ -z "${PLATFORMS}" ]]; then
    PLATFORMS="linux/$(docker version --format '{{.Server.Arch}}')"
fi
if [[ ${#TAGS[@]} -eq 0 ]]; then
    TAGS=(--tag "ecflow-server-dev:local")
fi

# ---------------------------------------------------------------------------
# Check that the package of every target platform is available
# ---------------------------------------------------------------------------

IFS=',' read -r -a PLATFORM_LIST <<< "${PLATFORMS}"
for platform in "${PLATFORM_LIST[@]}"; do
    arch="${platform#*/}"
    arch="${arch%%/*}"
    packages=("${CONTEXT}"/ecflow-*"-${arch}.deb")
    if [[ ! -f "${packages[0]}" ]]; then
        echo "No package for ${platform}: no ${CONTEXT}/<package>-${arch}.deb found (see build_ecflow_package.sh)" >&2
        exit 1
    fi
done

# ---------------------------------------------------------------------------
# Smoke test, then create the image for all platforms
# ---------------------------------------------------------------------------

if [[ "${SMOKE_TEST}" == "true" ]]; then
    for platform in "${PLATFORM_LIST[@]}"; do
        smoke_test "${platform}"
    done
fi

if [[ "${PUSH}" == "true" ]]; then
    OUTPUT=(--push)
else
    OUTPUT=(--load)
fi

docker buildx build \
    --platform "${PLATFORMS}" \
    "${TAGS[@]}" \
    ${LABELS[@]+"${LABELS[@]}"} \
    ${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"} \
    "${OUTPUT[@]}" \
    "${CONTEXT}"
