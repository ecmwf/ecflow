# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

FROM debian:12.7-slim

RUN apt-get update \
    && apt-get install -y \
        nginx \
        openssl  \
    && rm -rf /var/lib/apt/lists/*

# The certificate is installed, or generated, when the container starts; the image holds no key
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod 0755 /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
