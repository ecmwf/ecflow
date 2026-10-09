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
# deliver the package as <package>-<arch>.deb (e.g. ecflow-server-arm64.deb), the
# name the image build (e.g. ecflow-server/Dockerfile) selects for its target
# platform. When the
# server is built with Aviso support, the Aviso client library it links (provided
# by the build environment, not by the package) is delivered beside the package,
# as aviso-ffi-<arch>.tar.gz, for the image build to install.
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

Builds an ecflow Debian package (e.g. the ecflow-server package, used by the
ecflow-server-dev image), inside the build environment, and delivers it as
<package>-<arch>.deb in the output directory.
ecbuild is expected beside the ecflow sources, in DIR/../ecbuild.

Options:
  --source DIR             ecflow sources to build (required)
  --preset NAME            CMake preset used to configure/build/package
                             (default: ${PRESET})
  --jobs N                 Parallel build jobs (default: ${JOBS})
  --build-dir DIR          Build tree, kept out of the sources, which may be
                             read-only (default: DIR/.deploy/build/<preset>)
  --output-dir DIR         Directory the <package>-<arch>.deb is delivered to
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
rm -f "${BUILD_DIR}"/ecflow-*.deb "${OUTPUT_DIR}"/ecflow-*.deb "${OUTPUT_DIR}"/aviso-ffi-*.tar.gz

cmake --build "${BUILD_DIR}" --target package

# ---------------------------------------------------------------------------
# Deliver the package as <package>-<arch>.deb, named after the package (e.g. ecflow-server, ecflow-ui) and its
# Debian architecture (e.g. amd64, arm64)
# ---------------------------------------------------------------------------

for pkg in "${BUILD_DIR}"/ecflow-*.deb; do
    name=$(dpkg-deb --field "${pkg}" Package)
    arch=$(dpkg-deb --field "${pkg}" Architecture)
    echo "Delivering $(basename "${pkg}") as ${OUTPUT_DIR}/${name}-${arch}.deb"
    cp "${pkg}" "${OUTPUT_DIR}/${name}-${arch}.deb"
done

# ---------------------------------------------------------------------------
# Deliver the Aviso client library, when the server is built with Aviso support, as aviso-ffi-<arch>.tar.gz
# ---------------------------------------------------------------------------

# The archive holds the library under its file name and its soname (e.g. libaviso_ffi.so.2.4.2 and
# libaviso_ffi.so.2), to be extracted into the library directory of the image. The need for the library is read
# from the server built, rather than from the configuration, so that whatever enabled or disabled Aviso is honoured;
# a build without the server (e.g. ecFlowUI only) needs no library.
needed=""
if [[ -f "${BUILD_DIR}/bin/ecflow_server" ]]; then
    needed=$(readelf -d "${BUILD_DIR}/bin/ecflow_server" | sed -n '/NEEDED/p')
fi
if [[ "${needed}" == *libaviso_ffi* ]]; then
    library=$(sed -n 's/^AVISO_FFI_LIBRARY:FILEPATH=//p' "${BUILD_DIR}/CMakeCache.txt")
    if [[ -z "${library}" || ! -f "${library}" ]]; then
        echo "ecflow_server needs libaviso_ffi, but AVISO_FFI_LIBRARY is not set in ${BUILD_DIR}/CMakeCache.txt" >&2
        exit 1
    fi
    library=$(readlink -f "${library}")
    soname=$(readelf -d "${library}" | sed -n 's/.*Library soname: \[\(.*\)\]/\1/p')
    staging=$(mktemp -d)
    cp "${library}" "${staging}/"
    ln -s "$(basename "${library}")" "${staging}/${soname}"
    echo "Delivering $(basename "${library}") (soname ${soname}) as ${OUTPUT_DIR}/aviso-ffi-${arch}.tar.gz"
    tar -czf "${OUTPUT_DIR}/aviso-ffi-${arch}.tar.gz" --owner=0 --group=0 \
        -C "${staging}" "$(basename "${library}")" "${soname}"
    rm -rf "${staging}"
fi
