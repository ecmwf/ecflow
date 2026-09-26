# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# An SFTP endpoint to an ecFlow workspace, mounted at /workspace, so that suite files can be delivered to an
# ecFlow server sharing that workspace with sftp or scp.

FROM debian:13.7-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        openssh-server \
    && rm -rf /var/lib/apt/lists/* \
    && rm -f /etc/ssh/ssh_host_* \
    && mkdir -p /run/sshd /etc/ssh/keys

COPY sshd_config /etc/ssh/sshd_config
COPY authorized-keys /usr/local/bin/authorized-keys
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod 0755 /usr/local/bin/authorized-keys /usr/local/bin/entrypoint.sh

EXPOSE 22

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
