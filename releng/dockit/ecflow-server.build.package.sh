#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# ecflow-server.build.package.sh
#
# Checks out ecflow and ecbuild, and builds the ecflow Debian package used by
# the ecflow-server-dev image, inside a Docker container, reproducing the
# 'package' job of ecflow/.github/workflows/dockit.yml for the architecture of
# the Docker host.
#
# The script runs on the host: it launches the build environment container,
# performs the checkout, configure, build and package steps inside it, and
# copies the package out as ecflow-<arch>.deb (e.g. ecflow-arm64.deb), the name
# the image build (ecflow-server/Dockerfile) selects for its target platform.
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
PRESET="linux.gcc.server.release"
JOBS=4

SANDBOX_DIR="${PWD}/ecflow-server.sandbox"
OUTPUT_DIR="${PWD}/ecflow-server"

ECFLOW_REPO="https://github.com/ecmwf/ecflow.git"
ECBUILD_REPO="https://github.com/ecmwf/ecbuild.git"
ECFLOW_BRANCH="develop"
ECBUILD_BRANCH="3.16.0"

SKIP_CHECKOUT="false"
VERBOSE="false"

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
Usage: ecflow-server.build.package.sh [options]

Checks out ecflow and ecbuild, and builds an ecflow Debian package inside a
Docker container. Leaves the package, named ecflow-<arch>.deb after its
architecture (e.g. ecflow-arm64.deb), in the output directory, which by
default is the build context of the ecflow-server-dev image.

Options:
  --build_dir DIR          Sandbox directory for git clones and build trees,
                             bind-mounted into the container; the sources
                             are checked out under DIR/source/{ecflow,ecbuild}
                             (default: \${PWD}/ecflow-server.sandbox)
  --output_dir DIR         Directory the ecflow-<arch>.deb is copied into
                             (default: \${PWD}/ecflow-server)
  --docker-image IMAGE     Docker image used as the build environment
                             (default: ${DOCKER_IMAGE})
  --preset NAME            CMake preset used to configure/build/package
                             (default: ${PRESET})
  --jobs N                 Parallel build jobs (default: ${JOBS})
  --ecflow-branch REF      ecflow branch/tag to checkout (default: ${ECFLOW_BRANCH})
  --ecbuild-branch REF     ecbuild branch/tag to checkout (default: ${ECBUILD_BRANCH})
  --ecflow-repo URL        ecflow git repository URL (default: ${ECFLOW_REPO})
  --ecbuild-repo URL       ecbuild git repository URL (default: ${ECBUILD_REPO})
  --skip-checkout          Use the sources already under DIR/source, without
                             cloning or fetching
  --verbose                Print every command executed (set -x)
  -h, --help               Show this help message and exit
EOF
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build_dir) SANDBOX_DIR="$2"; shift 2 ;;
        --output_dir) OUTPUT_DIR="$2"; shift 2 ;;
        --docker-image) DOCKER_IMAGE="$2"; shift 2 ;;
        --preset) PRESET="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --ecflow-branch) ECFLOW_BRANCH="$2"; shift 2 ;;
        --ecbuild-branch) ECBUILD_BRANCH="$2"; shift 2 ;;
        --ecflow-repo) ECFLOW_REPO="$2"; shift 2 ;;
        --ecbuild-repo) ECBUILD_REPO="$2"; shift 2 ;;
        --skip-checkout) SKIP_CHECKOUT="true"; shift ;;
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

# ---------------------------------------------------------------------------
# In-container script: checkout, configure, build and package ecflow.
#
# Mirrors the 'package' job of ecflow/.github/workflows/dockit.yml: same
# build environment image, preset and CUSTOM_DEBIAN_PACKAGE_VERSION scheme
# (<project version>_<sha>), and same ecflow-<arch>.deb naming.
# ---------------------------------------------------------------------------

CONTAINER_SCRIPT=$(cat <<INNER_EOF
set -e
set -x

SOURCE_DIR=/workspace/source
ECBUILD_DIR=\${SOURCE_DIR}/ecbuild
ECFLOW_DIR=\${SOURCE_DIR}/ecflow

mkdir -p "\${SOURCE_DIR}"

function checkout_repo() {
    local repo_url="\$1"
    local branch="\$2"
    local dest_dir="\$3"

    if [[ -d "\${dest_dir}/.git" ]]; then
        git -C "\${dest_dir}" fetch --depth 1 origin "\${branch}"
        git -C "\${dest_dir}" checkout --detach FETCH_HEAD
    else
        git clone --branch "\${branch}" --depth 1 "\${repo_url}" "\${dest_dir}"
    fi
}

function configure() {
    pushd "\${ECFLOW_DIR}"

    version=\$(grep -e '^project' CMakeLists.txt | sed 's/project( [a-zA-Z ]*\([0-9.]*\) )/\1/g')_\$(git rev-parse HEAD)

    cmake --preset ${PRESET} -DCUSTOM_DEBIAN_PACKAGE_VERSION=\${version}

    popd
}

function build() {
    pushd "\${ECFLOW_DIR}"

    cmake --build --preset ${PRESET} --parallel ${JOBS} --target all

    popd
}

function package() {
    pushd "\${ECFLOW_DIR}"

    # Remove packages left by previous runs, so that only the one built now is delivered
    rm -f \${ECFLOW_DIR}/.deploy/build/${PRESET}/ecflow-*.deb /workspace/output/ecflow-*.deb

    cmake --build --preset ${PRESET} --target package

    # Deliver the package as ecflow-<arch>.deb, named after its Debian architecture (e.g. amd64,
    # arm64), which is the name the image build selects for its target platform
    for pkg in \${ECFLOW_DIR}/.deploy/build/${PRESET}/ecflow-*.deb; do
        arch=\$(dpkg-deb --field "\${pkg}" Architecture)
        echo "Delivering \$(basename "\${pkg}") as ecflow-\${arch}.deb"
        cp "\${pkg}" "/workspace/output/ecflow-\${arch}.deb"
    done

    popd
}

if [[ "${SKIP_CHECKOUT}" != "true" ]]; then
    checkout_repo "${ECBUILD_REPO}" "${ECBUILD_BRANCH}" "\${ECBUILD_DIR}"
    checkout_repo "${ECFLOW_REPO}" "${ECFLOW_BRANCH}" "\${ECFLOW_DIR}"
fi

configure
build
package
INNER_EOF
)

# ---------------------------------------------------------------------------
# Run the build environment
# ---------------------------------------------------------------------------

make_banner "Building ecflow Debian package (preset: ${PRESET}) in ${DOCKER_IMAGE}"

docker run --rm \
    -v "${SANDBOX_DIR}:/workspace" \
    -w /workspace \
    "${DOCKER_IMAGE}" \
    bash -c "${CONTAINER_SCRIPT}"

cp "${SANDBOX_DIR}"/output/ecflow-*.deb "${OUTPUT_DIR}/"

make_banner "ecflow Debian package(s) available in ${OUTPUT_DIR}"
ls -la "${OUTPUT_DIR}"/ecflow-*.deb
