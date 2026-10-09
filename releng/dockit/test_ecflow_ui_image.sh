#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Exercise the installed ecFlowUI image, including its entrypoint: ecFlowUI must show its window on an X server
# (Xvfb, in a helper container), run as the given UID/GID with its configuration on a bind mount, reach the X
# server directly when a SOCKS proxy is set (-pc4), and stop on request.
# Usage: bash test_ecflow_ui_image.sh IMAGE PLATFORM
set -euo pipefail

image=${1:?An image is required}
platform=${2:?A platform is required}

suffix="$$-${RANDOM}"
network="ecflow-ui-smoke-${suffix}"
xserver="ecflow-ui-smoke-x-${suffix}"
ui="ecflow-ui-smoke-ui-${suffix}"
network_created="false"
xserver_started="false"
ui_started="false"
config=""

cleanup() {
    if [[ "${ui_started}" == "true" ]]; then
        docker rm -f "${ui}" >/dev/null 2>&1 || true
    fi
    if [[ "${xserver_started}" == "true" ]]; then
        docker rm -f "${xserver}" >/dev/null 2>&1 || true
    fi
    if [[ "${network_created}" == "true" ]]; then
        docker network rm "${network}" >/dev/null 2>&1 || true
    fi
    if [[ -n "${config}" ]]; then
        rm -rf "${config}"
    fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

fail() {
    echo "Image test failed (${platform}): $*" >&2
    if [[ "${ui_started}" == "true" ]]; then
        docker logs "${ui}" 2>&1 | grep -v '^DBG:' >&2 || true
    fi
    exit 1
}

# The X server runs natively, in a helper container; the image under test, possibly emulated, reaches it over TCP

docker network create "${network}" >/dev/null
network_created="true"

docker run -d --name "${xserver}" --network "${network}" --network-alias xserver debian:13.7-slim \
    bash -c 'apt-get update -qq \
             && apt-get install -y -qq --no-install-recommends xvfb x11-utils > /dev/null \
             && exec Xvfb :0 -listen tcp -ac -screen 0 1280x800x24' >/dev/null
xserver_started="true"

for _ in $(seq 1 120); do
    if docker exec "${xserver}" xdpyinfo -display :0 >/dev/null 2>&1; then
        break
    fi
    sleep 2
done
docker exec "${xserver}" xdpyinfo -display :0 >/dev/null 2>&1 || fail "the X server did not start"

# ecFlowUI is started as the current user, with the configuration directory on a bind mount (as docker_ecflow_ui
# does), and with a SOCKS proxy at a port where nothing listens: the window can only show when the connection to
# the X server is excluded from the proxy

config=$(mktemp -d "${TMPDIR:-/tmp}/ecflow-ui-smoke.XXXXXXXXXX")
uid=$(id -u)
gid=$(id -g)

docker run -d --name "${ui}" --platform "${platform}" --network "${network}" \
    --env DISPLAY=xserver:0 \
    --env "ECFLOW_UID=${uid}" --env "ECFLOW_GID=${gid}" \
    --env ECFLOWUI_CONFIG_DIR=/config --volume "${config}:/config" \
    --env ECFLOWUI_SOCKS_PROXY=xserver:1080 \
    "${image}" -log -pc4 >/dev/null
ui_started="true"

shown="false"
for _ in $(seq 1 90); do
    if docker exec "${xserver}" xwininfo -display :0 -root -tree 2>/dev/null | grep -q '"ecFlowUI'; then
        shown="true"
        break
    fi
    if [[ "$(docker inspect -f '{{.State.Status}}' "${ui}")" != "running" ]]; then
        fail "the container stopped before showing the ecFlowUI window"
    fi
    sleep 2
done
[[ "${shown}" == "true" ]] || fail "the ecFlowUI window did not show"
echo "${platform}: ecFlowUI window shown"

# ecFlowUI runs as the given user, and writes its configuration to the bind mount as that user

[[ "$(docker exec "${ui}" id -u ecflow)" == "${uid}" ]] || fail "ecflow does not have UID ${uid}"
[[ -d "${config}/default.session" ]] || fail "no configuration was written to the bind mount"
owner=$(stat -c %u "${config}/default.session" 2>/dev/null || stat -f %u "${config}/default.session")
[[ "${owner}" == "${uid}" ]] || fail "the configuration is owned by UID ${owner}, not ${uid}"
echo "${platform}: ecFlowUI runs as UID ${uid}, configuration written to the bind mount"

# The proxy configuration excludes the X server

docker exec "${ui}" sh -c 'cat /tmp/runtime-*/proxychains.conf' | grep -q '^localnet .*:6000/255.255.255.255$' \
    || fail "the proxy configuration does not exclude the X server"

# The container stops on request, without having to be killed

docker stop -t 20 "${ui}" >/dev/null
status=$(docker inspect -f '{{.State.ExitCode}}' "${ui}")
[[ "${status}" != "137" ]] || fail "the container had to be killed to stop"
echo "${platform}: container stopped on request (exit status ${status})"

echo "${platform}: image test passed"
