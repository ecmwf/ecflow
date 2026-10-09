#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set -e            # Exit immediately if a command exits with a non-zero status
set -u            # Treat unset variables as an error when substituting
set -o pipefail   # Return the exit status of the last command in the pipe that failed

# Run ecFlowUI as the unprivileged user 'ecflow', with access to its configuration
#
# The container starts as root only to align the UID/GID of 'ecflow' with the owner of the (typically
# bind-mounted) configuration directory, so that ecFlowUI can write to it whatever UID owns it on the host.
# This script is then re-executed as 'ecflow'.

if [ "$(id -u)" = "0" ]; then
    config=${ECFLOWUI_CONFIG_DIR:-/home/ecflow/.ecflow_ui_v5}
    mkdir -p "${config}"

    if [ -n "${ECFLOW_UID:-}" ]; then
        # The UID/GID given explicitly (e.g. by docker_ecflow_ui) take precedence, as the owner of a bind mount
        # is not always visible in the container (e.g. Docker Desktop on macOS shows it as root)
        owner_uid=${ECFLOW_UID}
        owner_gid=${ECFLOW_GID:-${ECFLOW_UID}}
    else
        owner_uid=$(stat -c %u "${config}")
        owner_gid=$(stat -c %g "${config}")

        if [ "${owner_uid}" = "0" ]; then
            # A root-owned configuration directory (e.g. a bind-mounted host directory that Docker had to
            # create) is handed over to 'ecflow' when empty; one with contents is left untouched
            if [ -z "$(ls -A "${config}")" ]; then
                chown ecflow:ecflow "${config}"
            else
                echo "launch.sh: warning: ${config} is owned by root and not empty; it is left as is," \
                     "and may not be writable by ecflow (set ECFLOW_UID/ECFLOW_GID to choose the UID/GID)" >&2
            fi
        fi
    fi

    if [ "${owner_uid}" != "0" ]; then
        # Duplicate IDs are allowed, as the owner may share its UID/GID with a user/group of the image
        if [ "${owner_gid}" != "$(id -g ecflow)" ]; then
            groupmod --non-unique --gid "${owner_gid}" ecflow
        fi
        if [ "${owner_uid}" != "$(id -u ecflow)" ]; then
            usermod --non-unique --uid "${owner_uid}" --gid "${owner_gid}" ecflow
        fi
    fi

    export HOME=/home/ecflow USER=ecflow LOGNAME=ecflow
    exec setpriv --reuid=ecflow --regid=ecflow --init-groups "$0" "$@"
fi

# Qt expects a private runtime directory

export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/tmp/runtime-$(id -u)}
if [ ! -d "${XDG_RUNTIME_DIR}" ]; then
    mkdir -m 700 "${XDG_RUNTIME_DIR}"
fi

# The SOCKS proxy used by ecflow_ui -pc4, given as ECFLOWUI_SOCKS_PROXY=<host>:<port> (e.g. the local end of
# `ssh -D 9050`); proxychains only accepts a numeric address, so the host name is resolved here
#
# The connection to the X server must not go through the proxy: a display reached over TCP (e.g.
# host.docker.internal:0, or localhost:10.0 forwarded by `ssh -X`) is excluded with 'localnet', which only
# matches numeric addresses, so DISPLAY is rewritten with the resolved address of its host (with proxy_dns,
# a host name would otherwise be resolved by the proxy, on the remote side).

function resolve_ipv4() {
    getent ahostsv4 "$1" | awk 'NR == 1 { print $1 }'
}

if [ -n "${ECFLOWUI_SOCKS_PROXY:-}" ]; then
    proxy_host=${ECFLOWUI_SOCKS_PROXY%:*}
    proxy_port=${ECFLOWUI_SOCKS_PROXY##*:}
    proxy_address=$(resolve_ipv4 "${proxy_host}")
    if [ -z "${proxy_address}" ]; then
        echo "launch.sh: error: cannot resolve the SOCKS proxy host ${proxy_host}" >&2
        exit 1
    fi

    localnet=""
    display_host=""
    if [ -n "${DISPLAY:-}" ]; then
        display_host=${DISPLAY%:*}
        display_number=${DISPLAY##*:}
    fi
    if [ -n "${display_host}" ] && [ "${display_host}" != "unix" ]; then
        display_address=$(resolve_ipv4 "${display_host}")
        if [ -z "${display_address}" ]; then
            echo "launch.sh: error: cannot resolve the host of DISPLAY=${DISPLAY}" >&2
            exit 1
        fi
        export DISPLAY="${display_address}:${display_number}"
        localnet="localnet ${display_address}:$((6000 + ${display_number%%.*}))/255.255.255.255"
    fi

    export PROXYCHAINS_CONF_FILE="${XDG_RUNTIME_DIR}/proxychains.conf"
    cat > "${PROXYCHAINS_CONF_FILE}" <<EOF
strict_chain
proxy_dns
remote_dns_subnet 224
tcp_read_time_out 15000
tcp_connect_time_out 8000
${localnet}
[ProxyList]
socks5 ${proxy_address} ${proxy_port}
EOF
fi

# Any arguments given to the container are passed on to ecflow_ui (e.g. -log, or -ts <host> <port>)

exec /usr/local/bin/ecflow_ui "$@"
