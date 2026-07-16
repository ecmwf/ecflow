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
# An ECF_AUTHTOKENS already set in the environment this script runs in is equivalent to passing
# --authtokens with that value; the flag, if also given, takes precedence over it.
AUTHTOKENS="${ECF_AUTHTOKENS:-}"

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
  -h, --help               Show this help and exit

Prerequisites (macOS):
  - An X server (e.g. XQuartz) running, with 'xhost +' applied (this script also applies it).
  - If --proxychains=on, an SSH SOCKS tunnel bound to more than loopback, left open in another
    terminal:
        ssh -g -D 9050 user@hpc-gateway

Examples:
  ./docker_ecflow_ui.sh
  ./docker_ecflow_ui.sh --proxychains=on
  ./docker_ecflow_ui.sh -c ~/cfg/ecflow_ui -i ecflow-all-dev:2026.1
  ./docker_ecflow_ui.sh --authtokens ~/.ecflow_server.tokens
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

# proxychains=off bypasses the image's own ENTRYPOINT (launch-ecflow_ui.sh, which always sets up
# proxychains4) and runs ecflow_ui directly instead.
# Built as a single array, always with at least one element, rather than splicing in a separate
# (possibly empty) array of --entrypoint args: bash 3.2 (macOS's default /bin/bash) raises
# "unbound variable" under `set -u` when expanding "${arr[@]}" on a genuinely empty array.
DOCKER_ARGS=(
    --rm
    -P
    -e DISPLAY=host.docker.internal:0
    -v /tmp/.X11-unix:/tmp/.X11-unix
    -v "${CFG_DIR}:${CFG_DIR}"
)
if [ "${PROXYCHAINS}" = "off" ]; then
    DOCKER_ARGS+=(--entrypoint /usr/local/bin/ecflow_ui)
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

# No -i/-t: ecflow_ui is a GUI app driven over X11, not an interactive TTY program, and dropping
# them keeps this working when stdin is not a terminal - as when this script is itself piped into
# bash via curl. docker run still streams stdout/stderr to this terminal and blocks until exit.
exec docker run "${DOCKER_ARGS[@]}" --platform "${PLATFORM}" "${IMAGE}" -confd "${CFG_DIR}"
