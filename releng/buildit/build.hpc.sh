#!/usr/bin/env bash
#
# build.hpc.sh
#
# Standalone script that checks out, builds and installs ecflow
# (https://github.com/ecmwf/ecflow), together with its build-time
# dependency ecbuild (https://github.com/ecmwf/ecbuild).
#
# Designed to run on ECMWF HPC systems (Lmod 'module' environment) and to be
# runnable directly, without a prior checkout, e.g.:
#
#   curl -fsSL https://raw.githubusercontent.com/ecmwf/ecflow/develop/releng/buildit/build.hpc.sh \
#     | bash -s -- --build_dir /path/to/sandbox
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

CONFIGURATION="gcc8qt5"
BUILD_TYPE="RelWithDebInfo"
SANDBOX_DIR="${PWD}/sandbox"
INSTALL_DIR=""    # resolved after checkout: defaults to ${SANDBOX_DIR}/.install/<tag>
JOBS=8

ECFLOW_REPO="https://github.com/ecmwf/ecflow.git"
ECBUILD_REPO="https://github.com/ecmwf/ecbuild.git"
ECFLOW_BRANCH="develop"
ECBUILD_BRANCH="develop"

ENABLE_UI="ON"
ENABLE_PYTHON="ON"
ENABLE_UDP="ON"
ENABLE_HTTP="ON"
ENABLE_SSL="ON"
ENABLE_TESTS="ON"
ENABLE_ALL_TESTS="ON"
ENABLE_DOCS="OFF"

SKIP_CHECKOUT="false"
VERBOSE="false"

AVAILABLE_CONFIGURATIONS="gcc8 gcc8qt5 gcc9 gcc11 gcc12_2 gcc13_2 gcc14_2 gcc15_2 intel2021_4 intel2023_2 intel2025_0 intel2025_3 aocc4 nvidia24_1 nvidia24_11 nvidia25_11"

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
Usage: easy_install.md [options]

Checks out, builds and installs ecflow, together with its ecbuild build dependency.

Options:
  --configuration NAME               Compiler/toolchain configuration, loaded via
                                       'module' (default: ${CONFIGURATION})
                                       One of: ${AVAILABLE_CONFIGURATIONS}

  --build_type TYPE                  CMake build type: Debug|Release|RelWithDebInfo
                                       (default: ${BUILD_TYPE})

  --build_dir DIR                    Sandbox directory for git clones and build trees
                                       (default: \${PWD}/sandbox)

  --install_prefix DIR               Install prefix
                                       (default: \${build_dir}/.install/<tag>)

  --ecflow-branch REF                ecflow branch/tag to checkout (default: ${ECFLOW_BRANCH})

  --ecbuild-branch REF               ecbuild branch/tag to checkout (default: ${ECBUILD_BRANCH})

  --ecflow-repo URL                  ecflow git repository URL (default: ${ECFLOW_REPO})

  --ecbuild-repo URL                 ecbuild git repository URL (default: ${ECBUILD_REPO})

  --jobs N                           Parallel build jobs (default: ${JOBS})

  --enable-ui / --disable-ui         Build the Qt Viewer (default: ${ENABLE_UI})

  --enable-python / --disable-python Build Python bindings (default: ${ENABLE_PYTHON})

  --enable-tests / --disable-tests   Build test targets (default: ${ENABLE_TESTS})

  --skip-checkout                    Reuse an existing checkout in build_dir, do not clone/fetch

  --verbose                          Print every command executed (set -x)

  -h, --help                         Show this help message and exit
EOF
}

function load_utilities() {
    module load cmake/3.31.6
    module load ninja/1.12.1
    module load git/2.48.1
}

function load_gcc8() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/8.5.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.11.8-01
    module unload boost
    module load boost/1.84.0
    module unload qt
    module load qt/6.6.1
}

function load_gcc8qt5() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/8.5.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.11.10-01
    module unload boost
    module load boost/1.84.0
    module unload qt
    module load qt/5.12.0
}

function load_gcc9() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/9.3.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.8.8-01
    module unload boost
    module load boost/1.75.0
    module unload qt
    module load qt/6.2.0
}

function load_gcc11() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/11.2.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.8.8-01
    module unload boost
    module load boost/1.71.0
    module unload qt
    module load qt/6.2.0
}

function load_gcc12_2() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/12.2.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.10.10-01
    module unload boost
    module load boost/1.81.0
    module unload qt
    module load qt/6.2.0
}

function load_gcc13_2() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/13.2.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.11.8-01
    module unload boost
    module load boost/1.84.0
    module unload qt
    module load qt/6.6.1
}

function load_gcc14_2() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/14.2.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.12.9-01
    module unload boost
    module load boost/1.87.0
    module unload qt
    module load qt/6.6.1
}

function load_gcc15_2() {
    load_utilities
    module load prgenv/gnu
    module unload gcc
    module load gcc/15.2.0
    CC=gcc
    CXX=g++
    module unload python3
    module load python3/3.13.13-01
    module unload boost
    module load boost/1.90.0
    module unload qt
    module load qt/6.6.1
}

function load_intel2021_4() {
    load_utilities
    module load prgenv/intel
    module unload intel
    module load intel/2021.4.0
    CC=icc
    CXX=icpx
    module unload python3
    module load python3/3.10.10-01
    module unload boost
    module load boost/1.81.0
    module unload qt
    module load qt/6.2.0
}

function load_intel2023_2() {
    load_utilities
    module load prgenv/intel
    module unload intel
    module load intel/2023.2.0
    CC=icx
    CXX=icpx
    module unload python3
    module load python3/3.11.8-01
    module unload boost
    module load boost/1.84.0
    module unload qt
    module load qt/6.6.1
}

function load_intel2025_0() {
    load_utilities
    module load prgenv/intel-llvm
    module unload intel
    module load intel/2025.0.1
    CC=icx
    CXX=icpx
    module unload python3
    module load python3/3.12.9-01
    module unload boost
    module load boost/1.87.0
    module unload qt
    module load qt/6.6.1
}

function load_intel2025_3() {
    load_utilities
    module load prgenv/intel-llvm
    module unload intel
    module load intel/2025.3.1
    CC=icx
    CXX=icpx
    module unload python3
    module load python3/3.13.13-01
    module unload boost
    module load boost/1.90.0
    module unload qt
    module load qt/6.6.1
}

function load_aocc4() {
    load_utilities
    module load prgenv/amd
    module unload aocc
    module load aocc/4.0.0
    module unload python3
    module load python3/3.10.10-01
    module unload boost
    module load boost/1.81.0
    module unload qt
    module load qt/6.2.0
}

function load_nvidia24_1() {
    load_utilities
    module load prgenv/nvidia
    module unload nvidia
    module load nvidia/24.1
    CC=nvc
    CXX=nvc++
    module unload python3
    module load python3/3.11.8-01
    module unload boost
    module load boost/1.84.0
    module unload qt
    module load qt/5.12.0
}

function load_nvidia24_11() {
    load_utilities
    module load prgenv/nvidia
    module unload nvidia
    module load nvidia/24.11
    CC=nvc
    CXX=nvc++
    module unload python3
    module load python3/3.12.9-01
    module unload boost
    module load boost/1.87.0
    module unload qt
    module load qt/6.6.1
}

function load_nvidia25_11() {
    load_utilities
    module load prgenv/nvidia
    module unload nvidia
    module load nvidia/25.11
    CC=nvc
    CXX=nvc++
    module unload python3
    module load python3/3.13.13-01
    module unload boost
    module load boost/1.90.0
    module unload qt
    module load qt/6.6.1
}

function checkout_repo() {
    local repo_url="$1"
    local branch="$2"
    local dest_dir="$3"

    if [[ -d "${dest_dir}/.git" ]]; then
        make_banner "Updating existing checkout: ${dest_dir}"
        git -C "${dest_dir}" fetch --depth 1 origin "${branch}"
        git -C "${dest_dir}" checkout --detach FETCH_HEAD
    else
        make_banner "Cloning ${repo_url} (${branch}) into ${dest_dir}"
        git clone --branch "${branch}" --depth 1 "${repo_url}" "${dest_dir}"
    fi
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --configuration) CONFIGURATION="$2"; shift 2 ;;
        --build_type) BUILD_TYPE="$2"; shift 2 ;;
        --build_dir) SANDBOX_DIR="$2"; shift 2 ;;
        --install_prefix) INSTALL_DIR="$2"; shift 2 ;;
        --ecflow-branch) ECFLOW_BRANCH="$2"; shift 2 ;;
        --ecbuild-branch) ECBUILD_BRANCH="$2"; shift 2 ;;
        --ecflow-repo) ECFLOW_REPO="$2"; shift 2 ;;
        --ecbuild-repo) ECBUILD_REPO="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --enable-ui) ENABLE_UI="ON"; shift ;;
        --disable-ui) ENABLE_UI="OFF"; shift ;;
        --enable-python) ENABLE_PYTHON="ON"; shift ;;
        --disable-python) ENABLE_PYTHON="OFF"; shift ;;
        --enable-tests) ENABLE_TESTS="ON"; ENABLE_ALL_TESTS="ON"; shift ;;
        --disable-tests) ENABLE_TESTS="OFF"; ENABLE_ALL_TESTS="OFF"; shift ;;
        --skip-checkout) SKIP_CHECKOUT="true"; shift ;;
        --verbose) VERBOSE="true"; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

if [[ "${VERBOSE}" == "true" ]]; then
    set -x
fi

case "${BUILD_TYPE}" in
    Debug|Release|RelWithDebInfo) ;;
    *)
        echo "Invalid --build_type '${BUILD_TYPE}' (expected Debug|Release|RelWithDebInfo)" >&2
        exit 1
        ;;
esac

if ! declare -f "load_${CONFIGURATION}" >/dev/null; then
    echo "Unknown --configuration '${CONFIGURATION}'" >&2
    echo "Available configurations: ${AVAILABLE_CONFIGURATIONS}" >&2
    exit 1
fi

if ! command -v module >/dev/null 2>&1; then
    echo "The 'module' command is not available: this script requires an" >&2
    echo "Lmod-based environment (e.g. an ECMWF HPC login/compute node)." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Checkout
# ---------------------------------------------------------------------------

mkdir -p "${SANDBOX_DIR}"
SANDBOX_DIR="$(cd "${SANDBOX_DIR}" && pwd)"

# BUILD_TAG only depends on options already parsed above, so it can be
# derived before checkout/toolchain-loading, and used to align the default
# install directory with the build directory layout: ${SANDBOX_DIR}/.build/<tag>
# and ${SANDBOX_DIR}/.install/<tag>.
BUILD_TAG="linux.$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]').${CONFIGURATION}_$(uname -m)"
BUILD_DIR="${SANDBOX_DIR}/.build/${BUILD_TAG}"

if [[ -z "${INSTALL_DIR}" ]]; then
    INSTALL_DIR="${SANDBOX_DIR}/.install/${BUILD_TAG}"
fi

ECBUILD_SRC_DIR="${SANDBOX_DIR}/ecbuild"
ECFLOW_SRC_DIR="${SANDBOX_DIR}/ecflow"

if [[ "${SKIP_CHECKOUT}" != "true" ]]; then
    checkout_repo "${ECBUILD_REPO}" "${ECBUILD_BRANCH}" "${ECBUILD_SRC_DIR}"
    checkout_repo "${ECFLOW_REPO}" "${ECFLOW_BRANCH}" "${ECFLOW_SRC_DIR}"
fi

# ecflow's own CMakeLists.txt locates ecbuild via a HINTS path relative to
# itself (${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild), which is exactly where it
# has just been checked out above -- no extra CMake flag is needed for it.

ECFLOW_VERSION="$(grep -E '^project\(\s*ecflow' "${ECFLOW_SRC_DIR}/CMakeLists.txt" \
    | sed -E 's/.*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/')"

if [[ -z "${ECFLOW_VERSION}" ]]; then
    echo "Could not determine ecflow version from ${ECFLOW_SRC_DIR}/CMakeLists.txt" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Load toolchain
# ---------------------------------------------------------------------------

make_banner "Loading configuration: ${CONFIGURATION}"
load_${CONFIGURATION}

# ---------------------------------------------------------------------------
# Configure, build, install
# ---------------------------------------------------------------------------

CMAKE_ARGS=(
    -G Ninja
    -B "${BUILD_DIR}"
    -S "${ECFLOW_SRC_DIR}"
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DENABLE_UI="${ENABLE_UI}"
    -DENABLE_UDP="${ENABLE_UDP}"
    -DENABLE_HTTP="${ENABLE_HTTP}"
    -DENABLE_TESTS="${ENABLE_TESTS}"
    -DENABLE_ALL_TESTS="${ENABLE_ALL_TESTS}"
    -DENABLE_SSL="${ENABLE_SSL}"
    -DENABLE_DOCS="${ENABLE_DOCS}"
    -DENABLE_PYTHON="${ENABLE_PYTHON}"
)

if [[ "${ENABLE_PYTHON}" == "ON" ]]; then
    CMAKE_ARGS+=(
        -DCMAKE_PREFIX_PATH="$(python3 -c 'import pybind11; print(pybind11.get_cmake_dir())')"
        -DPython3_ROOT_DIR="$(python3 -c 'import sys; print(sys.exec_prefix)')"
        -DPython3_EXECUTABLE="$(python3 -c 'import sys; print(f"{sys.exec_prefix}/bin/python3")')"
        -DPython3_INCLUDE_DIRS="$(python3 -c 'import sys; print(f"{sys.exec_prefix}/include/python{sys.version_info[0]}.{sys.version_info[1]}")')"
        -DPython3_LIBRARIES="$(python3 -c 'import sys; print(f"{sys.exec_prefix}/lib/python{sys.version_info[0]}.{sys.version_info[1]}.so")')"
    )
fi

make_banner "Configure ecflow ${ECFLOW_VERSION} (${BUILD_TAG})"
cmake "${CMAKE_ARGS[@]}"

make_banner "Build ecflow ${ECFLOW_VERSION} (${BUILD_TAG})"
cmake --build "${BUILD_DIR}" -j "${JOBS}" --target all

make_banner "Install ecflow ${ECFLOW_VERSION} into ${INSTALL_DIR}"
cmake --build "${BUILD_DIR}" --target install

make_banner "ecflow ${ECFLOW_VERSION} installed into ${INSTALL_DIR}"
