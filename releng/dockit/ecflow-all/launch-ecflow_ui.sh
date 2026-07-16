#!/usr/bin/env bash

set -e
set -u
set -o pipefail

# Resolve the SOCKS5 proxy host (the SSH tunnel opened on the Docker host) to a literal IP address.
# proxychains-ng refuses a non-numeric address for the first proxy in a chain, since that hop is
# dialled directly, before any tunnel exists through which a hostname could be resolved remotely.
# The resolved IP is therefore generated at container start, not baked into the image.

PROXY_HOST=${PROXY_HOST:-host.docker.internal}
PROXY_PORT=${PROXY_PORT:-9050}

PROXY_HOST_IP=$(getent hosts "${PROXY_HOST}" | awk '{ print $1; exit }')
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

exec proxychains4 -f /etc/proxychains4.conf /usr/local/bin/ecflow_ui "$@"
