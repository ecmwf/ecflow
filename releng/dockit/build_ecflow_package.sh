#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# build_ecflow_package.sh
#
# Builds the ecflow Debian package used by the ecflow-server-dev image, for the
# architecture of the Docker host, by running build_ecflow_package_in_container.sh inside the
# build environment container. The 'package' job of
# ecflow/.github/workflows/dockit.yml runs the same build_ecflow_package_in_container.sh in
# its job container, so that both build the package in exactly the same way.
#
# The script runs on the host: it prepares the sandbox, launches the container,
# and copies the package out as ecflow-<arch>.deb (e.g. ecflow-arm64.deb), the
# name the image build (ecflow-server/Dockerfile) selects for its target platform.
#
# By default, ecflow is cloned; with --source DIR, the ecflow sources in DIR
# (e.g. a local working tree) are built instead.
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

DOCKER_IMAGE="marcosbento/lumen:debian-13.7"

SANDBOX_DIR="${PWD}/build_ecflow_package.sandbox"
OUTPUT_DIR="${PWD}/ecflow-server"

SOURCE=""
VERBOSE="false"

# The package build itself, shared with the dockit workflow; its defaults (preset, jobs, repositories and
# refs, including the pinned ecbuild tag) apply unless the corresponding options are given
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_SCRIPT="${SCRIPT_DIR}/build_ecflow_package_in_container.sh"
PACKAGE_ARGS=()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function make_banner() {
    echo "------------------------------------------------------------"
    echo " *** $@"
    echo "------------------------------------------------------------"
}

function usage() {
    cat <<EOF
Usage: build_ecflow_package.sh [options]

Builds an ecflow Debian package inside a Docker container, by running
build_ecflow_package_in_container.sh there. Leaves the package, named ecflow-<arch>.deb
after its architecture (e.g. ecflow-arm64.deb), in the output directory, which
by default is the build context of the ecflow-server-dev image.

Options:
  --build_dir DIR          Sandbox directory for git clones and build trees,
                             bind-mounted into the container; the sources
                             are checked out under DIR/source/{ecflow,ecbuild}
                             (default: \${PWD}/build_ecflow_package.sandbox)
  --output_dir DIR         Directory the ecflow-<arch>.deb is copied into
                             (default: \${PWD}/ecflow-server)
  --source DIR             Build the ecflow sources in DIR (mounted read-only)
                             instead of cloning ecflow; ecbuild is still
                             checked out (unless --skip-checkout)
  --docker-image IMAGE     Docker image used as the build environment
                             (default: ${DOCKER_IMAGE})
  --verbose                Print every command executed on the host (set -x)
  -h, --help               Show this help message and exit

Options passed on to build_ecflow_package_in_container.sh (defaults as it lists them):
  --preset NAME            CMake preset used to configure/build/package
  --jobs N                 Parallel build jobs
  --ecflow-branch REF      ecflow branch/tag to check out (without --source)
  --ecflow-repo URL        ecflow git repository URL (without --source)
  --ecbuild-branch REF     ecbuild branch/tag to check out
  --ecbuild-repo URL       ecbuild git repository URL
  --skip-checkout          Use the sources already under DIR/source, without
                             cloning or fetching
EOF
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build_dir) SANDBOX_DIR="$2"; shift 2 ;;
        --output_dir) OUTPUT_DIR="$2"; shift 2 ;;
        --source) SOURCE="$2"; shift 2 ;;
        --docker-image) DOCKER_IMAGE="$2"; shift 2 ;;
        --preset|--jobs|--ecflow-branch|--ecflow-repo|--ecbuild-branch|--ecbuild-repo)
            PACKAGE_ARGS+=("$1" "$2"); shift 2 ;;
        --skip-checkout) PACKAGE_ARGS+=("$1"); shift ;;
        --verbose) VERBOSE="true"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ "${VERBOSE}" == "true" ]]; then
    set -x
fi

if ! command -v docker >/dev/null 2>&1; then
    echo "docker is required to run the build environment, but was not found in PATH." >&2
    exit 1
fi

mkdir -p "${SANDBOX_DIR}"
SANDBOX_DIR="$(cd "${SANDBOX_DIR}" && pwd)"

mkdir -p "${OUTPUT_DIR}"
OUTPUT_DIR="$(cd "${OUTPUT_DIR}" && pwd)"

mkdir -p "${SANDBOX_DIR}/output"

# With --source, the ecflow sources are mounted (read-only) in place of a clone, and their commit is
# determined here, on the host, as their git metadata may not be usable inside the container (e.g. in a
# git submodule); otherwise, ecflow is cloned into the sandbox
DOCKER_SOURCE_ARGS=()
if [[ -n "${SOURCE}" ]]; then
    if [[ ! -f "${SOURCE}/CMakeLists.txt" ]]; then
        echo "--source ${SOURCE}: not an ecflow source directory (no CMakeLists.txt)" >&2
        exit 1
    fi
    SOURCE="$(cd "${SOURCE}" && pwd)"
    DOCKER_SOURCE_ARGS=(-v "${SOURCE}:/workspace/source/ecflow:ro")
    ECFLOW_SHA="$(git -C "${SOURCE}" rev-parse HEAD 2>/dev/null || true)"
    if [[ -n "${ECFLOW_SHA}" ]]; then
        PACKAGE_ARGS+=(--revision "${ECFLOW_SHA}")
    fi
else
    PACKAGE_ARGS+=(--clone-ecflow)
fi

# ---------------------------------------------------------------------------
# Run the build environment
# ---------------------------------------------------------------------------

make_banner "Building ecflow Debian package in ${DOCKER_IMAGE}"

# The container runs as the calling user, so that it can write to the sandbox (which it owns) and the
# results are owned by that user, also on Linux hosts
docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -v "${SANDBOX_DIR}:/workspace" \
    ${DOCKER_SOURCE_ARGS[@]+"${DOCKER_SOURCE_ARGS[@]}"} \
    -v "${PACKAGE_SCRIPT}:/opt/dockit/build_ecflow_package_in_container.sh:ro" \
    -w /workspace \
    "${DOCKER_IMAGE}" \
    bash /opt/dockit/build_ecflow_package_in_container.sh \
        --source /workspace/source/ecflow \
        --build-dir /workspace/build \
        --output-dir /workspace/output \
        ${PACKAGE_ARGS[@]+"${PACKAGE_ARGS[@]}"}

cp "${SANDBOX_DIR}"/output/ecflow-*.deb "${OUTPUT_DIR}/"

make_banner "ecflow Debian package(s) available in ${OUTPUT_DIR}"
ls -la "${OUTPUT_DIR}"/ecflow-*.deb
