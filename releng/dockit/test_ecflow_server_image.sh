#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Exercise the installed image, including its entrypoint, health check and persistent workspace.
# Usage: bash test_ecflow_server_image.sh IMAGE PLATFORM
set -euo pipefail

image=${1:?An image is required}
platform=${2:?A platform is required}
container=""
volume=""
config=""

cleanup() {
    if [[ -n "${container}" ]]; then
        docker rm -f "${container}" >/dev/null
    fi
    if [[ -n "${volume}" ]]; then
        docker volume rm "${volume}" >/dev/null
    fi
    if [[ -n "${config}" ]]; then
        docker volume rm "${config}" >/dev/null
        config=""
    fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

fail() {
    echo "Image lifecycle test failed: $*" >&2
    docker logs "${container}" >&2 || true
    exit 1
}

wait_healthy() {
    local _attempt state
    for _attempt in $(seq 1 30); do
        state=$(docker inspect -f '{{.State.Status}} {{.State.Health.Status}}' "${container}")
        [[ "${state}" == 'running healthy' ]] && return
        [[ "${state}" == exited* ]] && fail "server exited before becoming healthy"
        sleep 3
    done
    fail "server did not become healthy"
}

wait_exit() {
    local expected="$1" alternative="${2:-$1}" _attempt state code
    for _attempt in $(seq 1 30); do
        state=$(docker inspect -f '{{.State.Status}}' "${container}")
        if [[ "${state}" == exited ]]; then
            code=$(docker inspect -f '{{.State.ExitCode}}' "${container}")
            [[ "${code}" == "${expected}" || "${code}" == "${alternative}" ]] \
                || fail "expected exit ${expected}/${alternative}, got ${code}"
            return
        fi
        sleep 1
    done
    fail "server did not exit within 30 seconds"
}

client() {
    docker exec -e HOME=/home/ecflow -u ecflow "${container}" \
        ecflow_client --http --host localhost --port 8888 "$@"
}

new_workspace() {
    cleanup
    container=""
    volume=""
    volume=$(docker volume create)
    # Set ownership inside Docker, so the same test works on Linux and Docker Desktop.
    docker run --rm --network none --platform "${platform}" \
        --mount "type=volume,source=${volume},target=/workspace,volume-nocopy" \
        --entrypoint bash "${image}" -c 'chown "$1:$2" /workspace' -- "$1" "$2"
}

start() {
    container=$(docker run -d --network none --platform "${platform}" \
        --mount "type=volume,source=${volume},target=/workspace,volume-nocopy" "${image}" "$@")
}

# Remapped ownership and an empty root-owned workspace must both support persistence.
for owner in 501:20 0:0; do
    new_workspace "${owner%:*}" "${owner#*:}"
    start
    wait_healthy
    expected_owner=$(docker exec "${container}" stat -c '%u:%g' /workspace)
    if [[ "${owner}" == 501:20 ]]; then
        [[ "${expected_owner}" == "${owner}" ]] || fail "workspace ownership changed"
    fi
    docker exec "${container}" bash -c \
        'test "$(stat -c %u /workspace)" = "$(id -u ecflow)" && test "$(id -u ecflow)" != 0' \
        || fail "workspace was not assigned to the unprivileged server user"
    docker exec -u ecflow "${container}" bash -c \
        'printf "suite dockit_lifecycle\n  task t\nendsuite\n" > /tmp/lifecycle.def'
    client --load /tmp/lifecycle.def
    if [[ "${owner}" == 501:20 ]]; then
        docker stop --time 15 "${container}" >/dev/null
    else
        docker kill --signal INT "${container}" >/dev/null
    fi
    wait_exit 0
    # Inspect persisted files from a separate container while the server is stopped.
    docker run --rm --network none --platform "${platform}" \
        --mount "type=volume,source=${volume},target=/workspace,readonly,volume-nocopy" \
        --entrypoint bash "${image}" -c '
            set -eu
            for file in /workspace/ecflow-server.8888.ecf.check /workspace/ecflow-server.8888.ecf.log; do
                test -s "$file"
                test "$(stat -c %u:%g "$file")" = "$1"
            done' -- "${expected_owner}" || fail "checkpoint/log missing or incorrectly owned"
    docker start "${container}" >/dev/null
    wait_healthy
    suites=$(client --suites)
    [[ "${suites}" == *dockit_lifecycle* ]] || fail "suite was not recovered from the checkpoint"
    docker stop --time 15 "${container}" >/dev/null
    wait_exit 0
done

# The configuration and the checkpoint may be kept apart from the workspace: the server must read
# server_environment.cfg from ECFLOW_CONFIG_DIR, and write its checkpoint where ECF_CHECK points, while its
# log stays in the workspace (ECF_HOME).
new_workspace 501 20
config=$(docker volume create)
docker run --rm --network none --platform "${platform}" \
    --mount "type=volume,source=${config},target=/admin,volume-nocopy" \
    --entrypoint bash "${image}" -c 'printf "ECF_CHECKINTERVAL = 77\n" > /admin/server_environment.cfg'
container=$(docker run -d --network none --platform "${platform}" \
    --mount "type=volume,source=${volume},target=/workspace,volume-nocopy" \
    --mount "type=volume,source=${config},target=/admin,readonly,volume-nocopy" \
    --mount "type=tmpfs,target=/state,tmpfs-mode=1777" \
    -e ECFLOW_CONFIG_DIR=/admin \
    -e ECF_CHECK=/state/ecflow-server.8888.ecf.check -e ECF_CHECKOLD=/state/ecflow-server.8888.ecf.check.b \
    "${image}")
wait_healthy
client --stats | grep -Eq 'Check pt interval +77 ' || fail "server_environment.cfg was not read from ECFLOW_CONFIG_DIR"
client --check_pt
docker exec "${container}" bash -c '
    test -s /state/ecflow-server.8888.ecf.check &&
    test ! -e /workspace/ecflow-server.8888.ecf.check &&
    test -s /workspace/ecflow-server.8888.ecf.log' \
    || fail "checkpoint not kept apart from the workspace, or log not in the workspace"
docker stop --time 15 "${container}" >/dev/null
wait_exit 0
docker rm -f "${container}" >/dev/null
container=""
docker volume rm "${config}" >/dev/null
config=""

# A startup error must propagate through the launcher, rather than leave a running container.
new_workspace 501 20
start --dockit-invalid-option
wait_exit 134

# Signal the launcher as soon as its server process exists, without waiting for readiness.
# Either the terminate request succeeds or the documented kill fallback is used; neither may hang.
new_workspace 501 20
start
docker exec "${container}" bash -c '
    for attempt in {1..50}; do
        for file in /proc/[0-9]*/comm; do
            if [[ -r "$file" ]] && [[ "$(< "$file")" == ecflow_server ]]; then
                exit 0
            fi
        done
        sleep 0.1
    done
    exit 1' || fail "server process did not start"
docker kill --signal TERM "${container}" >/dev/null
wait_exit 0 137

# A populated, inaccessible root workspace must not be silently taken over.
new_workspace 0 0
docker run --rm --network none --platform "${platform}" \
    --mount "type=volume,source=${volume},target=/workspace,volume-nocopy" \
    --entrypoint bash "${image}" -c 'touch /workspace/keep; chmod 700 /workspace'
start
wait_exit 1

# A failed terminate command must take the explicit kill fallback and report failure.
new_workspace 501 20
start
wait_healthy
docker exec "${container}" chmod -x /usr/local/bin/ecflow_client
docker stop --time 15 "${container}" >/dev/null
wait_exit 137
docker logs "${container}" 2>&1 | grep -q 'terminate request failed; killing ecflow_server' \
    || fail "termination fallback was not used"

echo "${platform}: lifecycle, persistence, ownership and failure-path checks passed"
