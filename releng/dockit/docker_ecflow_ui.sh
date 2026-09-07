#!/usr/bin/env bash
#
# docker_ecflow_ui.sh - launch ecflow_ui in a Docker container (ecflow-all-dev:latest by default),
# with its X11 display forwarded to the host and, optionally, its connections routed through
# proxychains4 + an SSH SOCKS tunnel opened on the host (see ecflow-all/launch-ecflow_ui.sh).
#
# Designed to be run either as a local checkout, or fetched and piped straight into bash:
#
#   curl -fsSL <url>/docker_ecflow_ui.sh | bash -s -- --proxychains=on
#
# (arguments go after the `--`, per bash's own `-s` convention; with no `--`, no arguments can be
# passed and every default below applies.)

set -e
set -u
set -o pipefail

CFG_DIR="${HOME}/.ecflow_ui_v5"
IMAGE="eccr.ecmwf.int/ecflow-dev-environments/ecflow-all-dev:latest"
PLATFORM="linux/amd64"
PROXYCHAINS="off"
#
# When ECF_AUTHTOKENS is set in the environment this script runs, it is equivalent to passing
# --authtokens with that value; the flag, if also given, takes precedence over it.
#
AUTHTOKENS="${ECF_AUTHTOKENS:-}"
#
# Optional override for the host the container's entrypoint (launch-ecflow_ui.sh) uses as its SOCKS
# proxy / X11 / localnet address. Empty means: let the entrypoint resolve host.docker.internal
# itself (it forces IPv4). Set it (e.g. --proxy-host 192.168.65.254) to pin that address, for
# instance against an older image whose entrypoint still resolves an IPv6 address.
#
PROXY_HOST="${PROXY_HOST:-}"
#
# By default the container mirrors the invoking host user (same name/uid/gid): the launcher
# recreates that account inside the container and drops privileges to it, so ecflow reports the same
# login user as on the host (its get_login_name() is getpwuid(getuid())). --run-as-root keeps root.
#
RUN_AS_ROOT="false"
#
# Optional path to a host launcher script to mount over the image's baked-in entrypoint
# (launch-ecflow_ui.sh), so launcher fixes take effect without rebuilding the image.
# Empty means: use the launcher baked into the container as-is.
#
HOST_LAUNCHER=""

usage() {
    cat <<'EOF'
Usage: docker_ecflow_ui.sh [options]

Launch ecflow_ui in a Docker container, with its X11 display forwarded to the host and,
optionally, its connections routed through a proxychains4 + SSH SOCKS tunnel.

Options:
  -c, --cfg DIR           ecflow_ui configuration directory (default: ~/.ecflow_ui_v5)
  -i, --image IMAGE       Docker image to run (default: eccr.ecmwf.int/ecflow-dev-environments/ecflow-all-dev:latest)
      --platform PLATFORM Docker platform to use (default: linux/amd64)
      --proxychains=on|off
                           Route connections through proxychains4 + the SSH SOCKS tunnel baked
                           into the image's entrypoint (default: off)
      --authtokens FILE   Auth tokens file to mount into the container and expose to ecflow_ui
                           via ECF_AUTHTOKENS (default: unset). An ECF_AUTHTOKENS already set in
                           the calling environment is used as the default for this option, so
                           having it exported is equivalent to passing --authtokens explicitly;
                           an explicit --authtokens overrides it.
      --proxy-host HOST   Override the address the container's entrypoint uses for its SOCKS proxy
                           and X11/localnet exclusion, exposed as PROXY_HOST (default: unset, i.e.
                           the entrypoint resolves host.docker.internal itself, forcing IPv4). A
                           PROXY_HOST set in the calling environment is used as the default. Pin it
                           (e.g. 192.168.65.254) against an older image whose entrypoint still
                           resolves an IPv6 address and fails with "localnet address error".
      --run-as-root       Run the container as root instead of mirroring the invoking host user.
                           By default the host user's name/uid/gid are recreated inside the
                           container and ecflow_ui runs as that user, so ecflow reports the same
                           login user (getpwuid(getuid())) as on the host.
      --host-launcher FILE
                           Mount FILE (a host launcher script) over the image's baked-in entrypoint
                           launch-ecflow_ui.sh, so launcher fixes take effect without rebuilding the
                           image. Default: unset, i.e. the launcher baked into the container is used.
  -h, --help               Show this help and exit

Prerequisites (macOS):
  - An X server (e.g. XQuartz) running, with 'xhost +' applied (this script also applies it).
  - If --proxychains=on, an SSH SOCKS tunnel bound to more than loopback, left open in another
    terminal:
        ssh [-v] -g -C -N -D 9050 user@hpc-gateway
          -v for verbose output
          -g to allow remote hosts to connect (n.b. the container acts as a remote host)
          -C for compression
          -N to not run a command
          -D 9050 to open a SOCKS tunnel on port 9050 (the port baked into the image's entrypoint).

Notes:
  - With --host-launcher FILE, FILE is mounted over the image's entrypoint (both proxychains modes),
    allowing to customise the launcher without rebuilding the image.

Examples:
  ./docker_ecflow_ui.sh
  ./docker_ecflow_ui.sh --proxychains=on
  ./docker_ecflow_ui.sh -c ~/cfg/ecflow_ui -i ecflow-all-dev:2026.1
  ./docker_ecflow_ui.sh --authtokens ~/.ecflow_server.tokens
  ./docker_ecflow_ui.sh --host-launcher ./ecflow-all/launch-ecflow_ui.sh
  ECF_AUTHTOKENS=~/.ecflow_server.tokens ./docker_ecflow_ui.sh
  curl -fsSL <url>/docker_ecflow_ui.sh | bash -s -- --proxychains=on
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        -c|--cfg)
            CFG_DIR="$2"
            shift 2
            ;;
        --cfg=*)
            CFG_DIR="${1#*=}"
            shift
            ;;
        -i|--image)
            IMAGE="$2"
            shift 2
            ;;
        --image=*)
            IMAGE="${1#*=}"
            shift
            ;;
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --proxychains)
            PROXYCHAINS="$2"
            shift 2
            ;;
        --proxychains=*)
            PROXYCHAINS="${1#*=}"
            shift
            ;;
        --authtokens)
            AUTHTOKENS="$2"
            shift 2
            ;;
        --authtokens=*)
            AUTHTOKENS="${1#*=}"
            shift
            ;;
        --proxy-host)
            PROXY_HOST="$2"
            shift 2
            ;;
        --proxy-host=*)
            PROXY_HOST="${1#*=}"
            shift
            ;;
        --run-as-root)
            RUN_AS_ROOT="true"
            ;;
        --host-launcher)
            HOST_LAUNCHER="$2"
            shift 2
            ;;
        --host-launcher=*)
            HOST_LAUNCHER="${1#*=}"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "docker_ecflow_ui.sh: unknown option '$1'" >&2
            usage >&2
            exit 1
            ;;
    esac
done

case "${PROXYCHAINS}" in
    on|off) ;;
    *)
        echo "docker_ecflow_ui.sh: --proxychains must be 'on' or 'off', got '${PROXYCHAINS}'" >&2
        exit 1
        ;;
esac

if ! command -v docker >/dev/null 2>&1; then
    echo "docker_ecflow_ui.sh: docker is not installed or not on PATH" >&2
    exit 1
fi

mkdir -p "${CFG_DIR}"
CFG_DIR=$(cd "${CFG_DIR}" && pwd)

if command -v xhost >/dev/null 2>&1; then
    xhost + >/dev/null
else
    echo "docker_ecflow_ui.sh: warning: 'xhost' not found; skipping X11 access control setup" >&2
fi

#
# The image's ENTRYPOINT (launch-ecflow_ui.sh) is used in both modes. proxychains=off disables the
# proxychains setup via the environment rather than replacing the entrypoint, so the launcher can
# still recreate the run-as-user account and drop privileges.
#
DOCKER_ARGS=(
    --rm
    -P
    -e DISPLAY=host.docker.internal:0
    -v /tmp/.X11-unix:/tmp/.X11-unix
    -v "${CFG_DIR}:${CFG_DIR}"
)
if [ "${PROXYCHAINS}" = "off" ]; then
    DOCKER_ARGS+=(-e ECF_UI_NO_PROXYCHAINS=1)
fi

#
# Setup the host user inside the container (default), so that ecflow reports the same login user
# as on the host. The launcher recreates this account and drops privileges to it.
#   --run-as-root skips this and leaves the container as root.
#
if [ "${RUN_AS_ROOT}" = "false" ]; then
    DOCKER_ARGS+=(
        -e "ECF_RUN_USER=$(id -un)"
        -e "ECF_RUN_UID=$(id -u)"
        -e "ECF_RUN_GID=$(id -g)"
    )
fi

#
# When --host-launcher FILE is given, override the image's baked-in entrypoint (launch-ecflow_ui.sh)
# with FILE, so launcher fixes take effect without rebuilding the image. The launcher is the
# entrypoint in both proxychains modes. When unset, the launcher baked into the container is used.
#
if [ -n "${HOST_LAUNCHER}" ]; then
    if [ ! -f "${HOST_LAUNCHER}" ]; then
        echo "docker_ecflow_ui.sh: --host-launcher file '${HOST_LAUNCHER}' does not exist" >&2
        exit 1
    fi
    HOST_LAUNCHER_DIR=$(cd "$(dirname "${HOST_LAUNCHER}")" && pwd)
    HOST_LAUNCHER="${HOST_LAUNCHER_DIR}/$(basename "${HOST_LAUNCHER}")"
    echo "docker_ecflow_ui.sh: using host launcher (overriding image entrypoint): ${HOST_LAUNCHER}" >&2
    DOCKER_ARGS+=(-v "${HOST_LAUNCHER}:/opt/local/bin/launch-ecflow_ui.sh:ro")
fi

if [ -n "${AUTHTOKENS}" ]; then
    if [ ! -f "${AUTHTOKENS}" ]; then
        echo "docker_ecflow_ui.sh: --authtokens file '${AUTHTOKENS}' does not exist" >&2
        exit 1
    fi
    AUTHTOKENS_DIR=$(cd "$(dirname "${AUTHTOKENS}")" && pwd)
    AUTHTOKENS="${AUTHTOKENS_DIR}/$(basename "${AUTHTOKENS}")"
    # Mounted read-only, at the same path inside the container as on the host, so ECF_AUTHTOKENS
    # can be set to that one path without needing to translate it.
    DOCKER_ARGS+=(-v "${AUTHTOKENS}:${AUTHTOKENS}:ro" -e "ECF_AUTHTOKENS=${AUTHTOKENS}")
fi

# Only passed when set, so the container's entrypoint keeps resolving host.docker.internal (IPv4)
# by default; an explicit --proxy-host pins PROXY_HOST instead.
if [ -n "${PROXY_HOST}" ]; then
    DOCKER_ARGS+=(-e "PROXY_HOST=${PROXY_HOST}")
fi

# No -i/-t: ecflow_ui is a GUI app driven over X11, not an interactive TTY program, and dropping
# them keeps this working when stdin is not a terminal - as when this script is itself piped into
# bash via curl. docker run still streams stdout/stderr to this terminal and blocks until exit.
exec docker run "${DOCKER_ARGS[@]}" --platform "${PLATFORM}" "${IMAGE}" -confd "${CFG_DIR}"
