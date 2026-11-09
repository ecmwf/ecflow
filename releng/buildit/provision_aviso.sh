#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# provision_aviso.sh
#
# Provisions the Aviso client library (libaviso_ffi), required to build ecFlow with ENABLE_AVISO=ON, by
# downloading the prebuilt aviso-ffi release archive of aviso-client (https://github.com/ecmwf/aviso-client)
# for the platform it runs on, verifying its SHA-256 checksum, and unpacking it into a destination directory.
#
# On success, the directory of the unpacked release (i.e. the directory containing include/aviso_ffi/ and
# lib/) is the only output written to stdout, so that it can be given directly to CMake, e.g.:
#
#   AVISO_FFI_ROOT="$(./provision_aviso.sh --dest /path/to/deps)"
#   cmake ... -DENABLE_AVISO=ON -DAVISO_FFI_ROOT="${AVISO_FFI_ROOT}"
#
# All progress and error messages are written to stderr. A release already unpacked in the destination
# directory is reused, without being downloaded again.
#
# Used as a preliminary step of the CI builds (see .ci/runner/build.sh.j2 and .ci/hpc/build.sh.j2).
#
# Run with --help to see all available options.
#

set -e
set -u
set -o pipefail

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------

# Keep at, or above, the minimum version required by ecFlow (see cmake/Dependencies.cmake)
VERSION="2.4.3"
DEST_DIR="${TMPDIR:-/tmp}/aviso-ffi"
BASE_URL="https://github.com/ecmwf/aviso-client/releases/download"
SHA256=""

# ---------------------------------------------------------------------------
# Known release checksums, as published in the SHA256SUMS file of each release
# ---------------------------------------------------------------------------

function known_sha256() {
    case "$1" in
        aviso-ffi-2.4.3-linux-x86_64.tar.gz)  echo "7ad042c0e13cb072a021b79a27117c81bba7770087fb1707b8c6fa52d83a41d1" ;;
        aviso-ffi-2.4.3-linux-aarch64.tar.gz) echo "f2171d08dcdf7a2d213ebab527e0bb1eaf2967e397c22ff7af6009939bc8071e" ;;
        aviso-ffi-2.4.3-macos-x86_64.tar.gz)  echo "a7643463211dd1b0636bf023f22e00c95252804164b7580de0d837dbc60da0b5" ;;
        aviso-ffi-2.4.3-macos-arm64.tar.gz)   echo "6e77d1e0d7913a96d1e11c534c6f7b801c63ddcf2417392a6ad3c34da281e789" ;;
        aviso-ffi-2.4.2-linux-x86_64.tar.gz)  echo "3abaeec6c808a2ec0fd64542718641e406543b4f3605b8399fe3ac43b0d96bea" ;;
        aviso-ffi-2.4.2-linux-aarch64.tar.gz) echo "bb84509724f0818bfffb282aa2725701958a595fca69c42888aec6fa72ae5041" ;;
        aviso-ffi-2.4.2-macos-x86_64.tar.gz)  echo "a7703b6cb2a71fd629426642f8234a47725bcd8d5e326743a05580a60a6e8d8e" ;;
        aviso-ffi-2.4.2-macos-arm64.tar.gz)   echo "a043f66e76fc72b9d9cda5fb8079efb4b4d530d1e27b9b1bd93bb199efef2661" ;;
        *) echo "" ;;
    esac
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function usage() {
    cat <<EOF
Usage: provision_aviso.sh [options]

Downloads, verifies and unpacks the prebuilt Aviso client library (aviso-ffi release archive), and prints the
directory of the unpacked release, to be used as AVISO_FFI_ROOT when configuring ecFlow with ENABLE_AVISO=ON.

Options:
  --version VERSION    aviso-client release to provision (default: ${VERSION})

  --dest DIR           Directory where the release is unpacked
                         (default: \${TMPDIR:-/tmp}/aviso-ffi)

  --sha256 CHECKSUM    Expected SHA-256 checksum of the release archive
                         (default: the known checksum of the release, required for any other release)

  --base-url URL       Base URL of the release downloads (default: ${BASE_URL})

  -h, --help           Show this help message and exit

Supported platforms:
  - Linux (x86_64 and aarch64, glibc 2.28 or later)
  - macOS (x86_64 and arm64)
EOF
}

function log() {
    echo "provision_aviso: $*" >&2
}

function die() {
    echo "provision_aviso: ERROR: $*" >&2
    exit 1
}

function detect_platform() {
    local os arch
    case "$(uname -s)" in
        Linux) os="linux" ;;
        Darwin) os="macos" ;;
        *) die "no aviso-ffi release available for operating system $(uname -s)" ;;
    esac
    case "$(uname -m)" in
        x86_64 | amd64) arch="x86_64" ;;
        aarch64 | arm64) [[ "${os}" == "macos" ]] && arch="arm64" || arch="aarch64" ;;
        *) die "no aviso-ffi release available for architecture $(uname -m)" ;;
    esac
    echo "${os}-${arch}"
}

function download() { # url destination
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL --retry 5 --retry-delay 5 --connect-timeout 30 -o "$2" "$1"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --tries=5 --waitretry=5 --timeout=30 -O "$2" "$1"
    else
        die "neither curl nor wget is available to download $1"
    fi
}

function sha256_of() { # file
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | cut -d' ' -f1
    else
        die "neither sha256sum nor shasum is available to verify $1"
    fi
}

function is_provisioned() { # root
    [[ -f "$1/include/aviso_ffi/aviso.hpp" ]] && compgen -G "$1/lib/libaviso_ffi.*" >/dev/null
}

# ---------------------------------------------------------------------------
# Parse command line arguments
# ---------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) VERSION="${2:?--version requires a value}"; shift 2 ;;
        --dest) DEST_DIR="${2:?--dest requires a value}"; shift 2 ;;
        --sha256) SHA256="${2:?--sha256 requires a value}"; shift 2 ;;
        --base-url) BASE_URL="${2:?--base-url requires a value}"; shift 2 ;;
        -h | --help) usage; exit 0 ;;
        *) usage >&2; die "unknown option '$1'" ;;
    esac
done

# ---------------------------------------------------------------------------
# Provision
# ---------------------------------------------------------------------------

PLATFORM="$(detect_platform)"
NAME="aviso-ffi-${VERSION}-${PLATFORM}"
ARCHIVE="${NAME}.tar.gz"
URL="${BASE_URL}/${VERSION}/${ARCHIVE}"

mkdir -p "${DEST_DIR}"
DEST_DIR="$(cd "${DEST_DIR}" && pwd)"
ROOT="${DEST_DIR}/${NAME}"

if is_provisioned "${ROOT}"; then
    log "reusing aviso-ffi ${VERSION} (${PLATFORM}) at ${ROOT}"
    echo "${ROOT}"
    exit 0
fi

if [[ -z "${SHA256}" ]]; then
    SHA256="$(known_sha256 "${ARCHIVE}")"
    [[ -n "${SHA256}" ]] || die "no known checksum for ${ARCHIVE}; provide one with --sha256"
fi

WORK_DIR="$(mktemp -d "${DEST_DIR}/.provision.XXXXXX")"
trap 'rm -rf "${WORK_DIR}"' EXIT

log "downloading ${URL}"
download "${URL}" "${WORK_DIR}/${ARCHIVE}" || die "unable to download ${URL}"

ACTUAL_SHA256="$(sha256_of "${WORK_DIR}/${ARCHIVE}")"
[[ "${ACTUAL_SHA256}" == "${SHA256}" ]] \
    || die "checksum mismatch for ${ARCHIVE} (expected ${SHA256}, found ${ACTUAL_SHA256})"
log "verified SHA-256 checksum ${ACTUAL_SHA256}"

# Only accept archives whose entries are all located under the expected top-level directory
if tar -tzf "${WORK_DIR}/${ARCHIVE}" | grep -v -E "^(\./)?${NAME}(/|$)" | grep -q .; then
    die "unexpected entries in ${ARCHIVE}, outside of ${NAME}/"
fi
if tar -tzf "${WORK_DIR}/${ARCHIVE}" | grep -q -E '(^|/)\.\.(/|$)'; then
    die "unsafe entries in ${ARCHIVE}"
fi

tar -xzf "${WORK_DIR}/${ARCHIVE}" -C "${WORK_DIR}" --no-same-owner
is_provisioned "${WORK_DIR}/${NAME}" || die "${ARCHIVE} does not hold include/aviso_ffi/aviso.hpp and lib/libaviso_ffi.*"

# The static library is not used by ecFlow, and is by far the largest file of the release
rm -f "${WORK_DIR}/${NAME}/lib/libaviso_ffi.a"

rm -rf "${ROOT}"
mv "${WORK_DIR}/${NAME}" "${ROOT}"

log "provisioned aviso-ffi ${VERSION} (${PLATFORM}) at ${ROOT}"
echo "${ROOT}"
