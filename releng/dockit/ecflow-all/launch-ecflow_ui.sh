#!/usr/bin/env bash

set -e
set -u
set -o pipefail

#
# Allow to, by default, run ecflow_ui as a specific (host) user instead of root.
# This can be opted out of by setting ECF_UI_NO_PROXYCHAINS, which will skip all proxychains setup and run ecflow_ui straight away, under the optional privilege drop.
#
# When ECF_RUN_UID is set, a matching group/passwd entry is created and privileges are dropped to
# it (via setpriv) before ecflow_ui is exec'd, so that ecflow's get_login_name() -- which is just
# getpwuid(getuid())->pw_name, ignoring $USER/$LOGNAME -- reports that user, mirroring how the
# client identifies the user on the host. root is retained until the final exec because writing
# /etc/proxychains4.conf below requires it. ECF_RUN_USER is required when ECF_RUN_UID is set;
# ECF_RUN_GID defaults to ECF_RUN_UID.
#
run_as_user_prefix=()
if [ -n "${ECF_RUN_UID:-}" ]; then
    run_as_user="${ECF_RUN_USER:?launch-ecflow_ui.sh: ECF_RUN_USER must be set when ECF_RUN_UID is set}"
    run_as_gid="${ECF_RUN_GID:-${ECF_RUN_UID}}"
    if ! getent group "${run_as_gid}" >/dev/null; then
        echo "${run_as_user}:x:${run_as_gid}:" >> /etc/group
    fi
    if ! getent passwd "${ECF_RUN_UID}" >/dev/null; then
        echo "${run_as_user}:x:${ECF_RUN_UID}:${run_as_gid}::/home/${run_as_user}:/bin/sh" >> /etc/passwd
    fi
    install -d -o "${ECF_RUN_UID}" -g "${run_as_gid}" "/home/${run_as_user}"
    # The ecflow_ui wrapper uses these for its temp/log paths; setpriv does not set them.
    export HOME="/home/${run_as_user}" USER="${run_as_user}" LOGNAME="${run_as_user}"
    run_as_user_prefix=(setpriv --reuid "${ECF_RUN_UID}" --regid "${run_as_gid}" --init-groups)
fi

#
# When proxychains is not wanted (direct connections, i.e. --proxychains=off), skip all proxychains
# setup and run ecflow_ui straight away, under the optional privilege drop.
#
if [ -n "${ECF_UI_NO_PROXYCHAINS:-}" ]; then
    exec "${run_as_user_prefix[@]}" /usr/local/bin/ecflow_ui "$@"
fi

# Resolve the SOCKS5 proxy host (the SSH tunnel opened on the Docker host) to a literal IP address.
# proxychains-ng refuses a non-numeric address for the first proxy in a chain, since that hop is
# dialled directly, before any tunnel exists through which a hostname could be resolved remotely.
# The resolved IP is therefore generated at container start, not baked into the image.

PROXY_HOST=${PROXY_HOST:-host.docker.internal}
PROXY_PORT=${PROXY_PORT:-9050}

# Force IPv4 resolution: 'getent hosts' may return an IPv6 address first (recent Docker Desktop
# hands back e.g. fdc4:...::254 for host.docker.internal), but the proxychains SOCKS proxy line and
# the /24 'localnet' exclusion computed below both assume a dotted-quad IPv4 address - an IPv6
# result yields a malformed 'localnet ...' line and proxychains aborts with "localnet address error".
PROXY_HOST_IP=$(getent ahostsv4 "${PROXY_HOST}" | awk '{ print $1; exit }')
if [ -z "${PROXY_HOST_IP}" ]; then
    echo "launch-ecflow_ui.sh: could not resolve ${PROXY_HOST} to an IP address" >&2
    exit 1
fi

# Under LD_PRELOAD, proxychains4 intercepts every outbound connect() in the process, including
# traffic that has nothing to do with the HPC connection - notably the X11 connection back to
# host.docker.internal:0 for the Qt UI. host.docker.internal resolves to the same host/subnet as
# the proxy itself, so without this exclusion that local traffic gets shoved through the SOCKS
# tunnel too and times out. localnet routes matching traffic direct, bypassing the chain.
PROXY_HOST_SUBNET=$(echo "${PROXY_HOST_IP}" | awk -F. '{ print $1"."$2"."$3".0" }')

cat > /etc/proxychains4.conf <<EOF
strict_chain
proxy_dns
remote_dns_subnet 224

localnet 127.0.0.0/255.0.0.0
localnet ${PROXY_HOST_SUBNET}/255.255.255.0

[ProxyList]
socks5  ${PROXY_HOST_IP}  ${PROXY_PORT}
EOF

# proxy_dns intercepts every symbolic-hostname DNS lookup in the process and substitutes a fake
# address in remote_dns_subnet before the localnet check ever runs, so localnet only ever matches
# a connect() made with a literal numeric IP - never one that starts from a hostname. DISPLAY is
# typically set to a hostname (e.g. host.docker.internal:0), so it must be rewritten to the same
# literal IP already resolved above, or the localnet exclusion above never applies to it.
if [ -n "${DISPLAY:-}" ]; then
    export DISPLAY="${PROXY_HOST_IP}:${DISPLAY#*:}"
fi

exec "${run_as_user_prefix[@]}" proxychains4 -f /etc/proxychains4.conf /usr/local/bin/ecflow_ui "$@"
