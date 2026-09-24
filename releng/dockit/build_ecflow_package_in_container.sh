#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# build_ecflow_package_in_container.sh
#
# Builds the ecflow Debian package used by the ecflow-server-dev image, for the
# architecture of the machine it runs on. It runs inside the build environment,
# and is the single definition of the package build shared by:
#
#   - build_ecflow_package.sh, which runs it in a container on the host, and
#   - the 'package' job of ecflow/.github/workflows/dockit.yml, which runs it in
#     its job container,
#
# so that both build the package in exactly the same way.
#
# Steps: check out ecbuild beside the ecflow sources (and, with --clone-ecflow,
# the ecflow sources themselves), then configure, build and package ecflow, and
# deliver the package as ecflow-<arch>.deb (e.g. ecflow-arm64.deb), the name the
# image build (ecflow-server/Dockerfile) selects for its target platform.
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

SOURCE=""
PRESET="linux.gcc.server.release"
JOBS=4
BUILD_DIR=""
OUTPUT_DIR="${PWD}"
REVISION=""

CLONE_ECFLOW="false"
ECFLOW_REPO="https://github.com/ecmwf/ecflow.git"
ECFLOW_BRANCH="develop"
# Pinned, so that a change on ecbuild develop cannot change or break the build
ECBUILD_REPO="https://github.com/ecmwf/ecbuild.git"
ECBUILD_BRANCH="3.16.0"
SKIP_CHECKOUT="false"

PRINT_VERSION="false"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function usage() {
    cat <<EOF
Usage: build_ecflow_package_in_container.sh --source DIR [options]

Builds the ecflow Debian package used by the ecflow-server-dev image, inside the
build environment, and delivers it as ecflow-<arch>.deb in the output directory.
ecbuild is expected beside the ecflow sources, in DIR/../ecbuild.

Options:
  --source DIR             ecflow sources to build (required)
  --preset NAME            CMake preset used to configure/build/package
                             (default: ${PRESET})
  --jobs N                 Parallel build jobs (default: ${JOBS})
  --build-dir DIR          Build tree, kept out of the sources, which may be
                             read-only (default: DIR/.deploy/build/<preset>)
  --output-dir DIR         Directory the ecflow-<arch>.deb is delivered to
                             (default: the current directory)
  --revision SHA           Commit of the sources, used in the package file name
                             (default: the HEAD commit of the sources)
  --clone-ecflow           Check out ecflow into DIR, instead of building the
                             sources already there
  --ecflow-repo URL        ecflow git repository URL (default: ${ECFLOW_REPO})
  --ecflow-branch REF      ecflow branch/tag to check out (default: ${ECFLOW_BRANCH})
  --ecbuild-repo URL       ecbuild git repository URL (default: ${ECBUILD_REPO})
  --ecbuild-branch REF     ecbuild branch/tag to check out (default: ${ECBUILD_BRANCH})
  --skip-checkout          Use the sources (ecflow and ecbuild) as they are,
                             without cloning or fetching
  --print-version          Print the package version, <project version>_<revision>,
                             and exit (requires only DIR/CMakeLists.txt and git)
  -h, --help               Show this help message and exit
EOF
}

function checkout_repo() {
    local repo_url="$1"
    local branch="$2"
    local dest_dir="$3"

    if [[ -d "${dest_dir}/.git" ]]; then
        git -C "${dest_dir}" fetch --depth 1 origin "${branch}"
        git -C "${dest_dir}" checkout --detach FETCH_HEAD
    else
        git clone --branch "${branch}" --depth 1 "${repo_url}" "${dest_dir}"
    fi
}

# <project version>_<revision>, e.g. 5.19.0_<sha>, used in the package file name
function package_version() {
    local project_version
    project_version=$(grep -e '^project' "${SOURCE}/CMakeLists.txt" | sed 's/project( [a-zA-Z ]*\([0-9.]*\) )/\1/g')
    echo "${project_version}_${REVISION:-$(git -C "${SOURCE}" rev-parse HEAD)}"
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --source) SOURCE="$2"; shift 2 ;;
        --preset) PRESET="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --revision) REVISION="$2"; shift 2 ;;
        --clone-ecflow) CLONE_ECFLOW="true"; shift ;;
        --ecflow-repo) ECFLOW_REPO="$2"; shift 2 ;;
        --ecflow-branch) ECFLOW_BRANCH="$2"; shift 2 ;;
        --ecbuild-repo) ECBUILD_REPO="$2"; shift 2 ;;
        --ecbuild-branch) ECBUILD_BRANCH="$2"; shift 2 ;;
        --skip-checkout) SKIP_CHECKOUT="true"; shift ;;
        --print-version) PRINT_VERSION="true"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
done

if [[ -z "${SOURCE}" ]]; then
    echo "build_ecflow_package_in_container.sh: --source is required" >&2
    exit 1
fi

# The sources may be owned by another user than the one running this script (e.g. when mounted into a
# container); this configuration is passed to git through the environment, without writing any file
export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=safe.directory GIT_CONFIG_VALUE_0='*'

if [[ "${PRINT_VERSION}" == "true" ]]; then
    package_version
    exit 0
fi

set -x

mkdir -p "${SOURCE}" "${OUTPUT_DIR}"
SOURCE="$(cd "${SOURCE}" && pwd)"
OUTPUT_DIR="$(cd "${OUTPUT_DIR}" && pwd)"
BUILD_DIR="${BUILD_DIR:-${SOURCE}/.deploy/build/${PRESET}}"

# ecflow locates ecbuild through a hint on the directory beside its own source
ECBUILD_DIR="$(dirname "${SOURCE}")/ecbuild"

# ---------------------------------------------------------------------------
# Checkout
# ---------------------------------------------------------------------------

if [[ "${SKIP_CHECKOUT}" != "true" ]]; then
    checkout_repo "${ECBUILD_REPO}" "${ECBUILD_BRANCH}" "${ECBUILD_DIR}"
    if [[ "${CLONE_ECFLOW}" == "true" ]]; then
        checkout_repo "${ECFLOW_REPO}" "${ECFLOW_BRANCH}" "${SOURCE}"
    fi
fi

# ---------------------------------------------------------------------------
# Configure, build and package
# ---------------------------------------------------------------------------

version=$(package_version)

(cd "${SOURCE}" && cmake --preset "${PRESET}" -B "${BUILD_DIR}" -DCUSTOM_DEBIAN_PACKAGE_VERSION="${version}")

cmake --build "${BUILD_DIR}" --parallel "${JOBS}" --target all

# Remove packages left by previous runs, so that only the one built now is delivered
rm -f "${BUILD_DIR}"/ecflow-*.deb "${OUTPUT_DIR}"/ecflow-*.deb

cmake --build "${BUILD_DIR}" --target package

# ---------------------------------------------------------------------------
# Deliver the package as ecflow-<arch>.deb, named after its Debian architecture (e.g. amd64, arm64)
# ---------------------------------------------------------------------------

for pkg in "${BUILD_DIR}"/ecflow-*.deb; do
    arch=$(dpkg-deb --field "${pkg}" Architecture)
    echo "Delivering $(basename "${pkg}") as ${OUTPUT_DIR}/ecflow-${arch}.deb"
    cp "${pkg}" "${OUTPUT_DIR}/ecflow-${arch}.deb"
done
