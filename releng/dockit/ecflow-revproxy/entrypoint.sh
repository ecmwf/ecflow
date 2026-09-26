#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Installs the certificate of the reverse proxy, then runs nginx in the foreground.
#
# The certificate and its key are taken from REVPROXY_TLS_DIR (tls.crt and tls.key, as in a Kubernetes Secret of
# type kubernetes.io/tls) when provided; otherwise, a self-signed certificate for localhost is generated, anew at
# every start. Neither is part of the image, which can therefore be published.

set -euo pipefail

tls_dir=${REVPROXY_TLS_DIR:-/etc/nginx/tls}
certificate=/etc/ssl/certs/revproxy.crt
key=/etc/ssl/private/revproxy.key

if [[ -r "${tls_dir}/tls.crt" && -r "${tls_dir}/tls.key" ]]; then
    install -m 0644 "${tls_dir}/tls.crt" "${certificate}"
    install -m 0600 "${tls_dir}/tls.key" "${key}"
    echo "entrypoint: using the certificate provided in ${tls_dir}"
else
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost" \
        -keyout "${key}" -out "${certificate}" 2>/dev/null
    chmod 0600 "${key}"
    echo "entrypoint: no certificate in ${tls_dir}; generated a self-signed certificate for localhost"
fi

exec nginx -g "daemon off;error_log /dev/stdout info;"
