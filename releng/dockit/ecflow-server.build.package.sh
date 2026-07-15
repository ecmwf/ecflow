#!/usr/bin/env bash
#
# build.package.sh
#
# Checks out ecflow and ecbuild, and builds an ecflow Debian package inside a
# Docker container, reproducing the 'package' step of
# ecflow/.github/workflows/dockit.yml. The resulting ecflow-*.deb is left in
# the directory where this script was executed.
#
# Unlike build.workflow.sh (which assumes ecflow/ecbuild are already checked
# out under /workspace/source, e.g. by a CI checkout action, and runs inside
# the container), this script runs on the host: it launches the Docker
# container itself, performs the checkout inside it, and copies the packaged
# .deb back out.
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

DOCKER_IMAGE="marcosbento/lumen:debian-13.5"
PRESET="linux.gcc.serveronly.relwithdebinfo"
JOBS=4

SANDBOX_DIR="${PWD}/ecflow-server.sandbox"
OUTPUT_DIR="${PWD}/ecflow-server"

ECFLOW_REPO="https://github.com/ecmwf/ecflow.git"
ECBUILD_REPO="https://github.com/ecmwf/ecbuild.git"
ECFLOW_BRANCH="develop"
ECBUILD_BRANCH="develop"

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
Usage: build.package.sh [options]

Checks out ecflow and ecbuild, and builds an ecflow Debian package inside a
Docker container. Leaves the resulting ecflow-*.deb in the output directory
(by default, the directory where this script was executed).

Options:
  --build_dir DIR          Sandbox directory for git clones and build trees,
                             bind-mounted into the container
                             (default: \${PWD}/sandbox)
  --output_dir DIR         Directory the ecflow-*.deb is copied into
                             (default: \${PWD})
  --docker-image IMAGE     Docker image used as the build environment
                             (default: ${DOCKER_IMAGE})
  --preset NAME            CMake preset used to configure/build/package
                             (default: ${PRESET})
  --jobs N                 Parallel build jobs (default: ${JOBS})
  --ecflow-branch REF      ecflow branch/tag to checkout (default: ${ECFLOW_BRANCH})
  --ecbuild-branch REF     ecbuild branch/tag to checkout (default: ${ECBUILD_BRANCH})
  --ecflow-repo URL        ecflow git repository URL (default: ${ECFLOW_REPO})
  --ecbuild-repo URL       ecbuild git repository URL (default: ${ECBUILD_REPO})
  --skip-checkout          Reuse an existing checkout in build_dir, do not clone/fetch
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
# Mirrors the 'package' job of ecflow/.github/workflows/dockit.yml -- same
# preset, same CUSTOM_DEBIAN_PACKAGE_VERSION scheme (<project version>_<sha>)
# -- and the configure/build/package steps of build.workflow.sh, plus the
# checkout step that build.workflow.sh otherwise assumes has already happened.
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

    cmake --build --preset ${PRESET} --target package

    cp \${ECFLOW_DIR}/.deploy/build/${PRESET}/ecflow-*.deb /workspace/output/

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
