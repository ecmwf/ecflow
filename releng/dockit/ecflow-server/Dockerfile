# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

FROM debian:13.5-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        python3-venv \
        tini \
    && rm -rf /var/lib/apt/lists/*

# Provision ecFlow dependencies
RUN python3 -m venv /opt/local/python \
    && /opt/local/python/bin/pip install "troika>=0.2.6"

# Provision ecFlow package
# This Dockerfile assumes that the ecFlow package is available in the build context (i.e. this directory).
# If you are using a different version, change the ECFLOW_PACKAGE variable accordingly.
# The package is bind-mounted, rather than copied, so that it is not kept in an image layer, and installed
# with apt-get, so that the runtime libraries it depends on are installed as well.
ARG ECFLOW_VERSION="latest"
ARG ECFLOW_PACKAGE="ecflow-${ECFLOW_VERSION}-Linux.deb"
RUN --mount=type=bind,source=${ECFLOW_PACKAGE},target=/tmp/${ECFLOW_PACKAGE} \
    apt-get update \
    && apt-get install -y --no-install-recommends /tmp/${ECFLOW_PACKAGE} \
    && rm -rf /var/lib/apt/lists/*

ARG ECFLOW_LAUNCH="launch.sh"
COPY ${ECFLOW_LAUNCH} /opt/local/bin/${ECFLOW_LAUNCH}
RUN chmod +x /opt/local/bin/${ECFLOW_LAUNCH}

# Run as an unprivileged user
# ecFlow identifies users by their passwd entry, so the container must run as this user (not as an arbitrary
# `docker run --user` UID); ECFLOW_UID/ECFLOW_GID match it to the owner of a bind-mounted workspace. An existing
# group with ECFLOW_GID (e.g. GID 20, the default group on macOS) is reused.
ARG ECFLOW_UID=1000
ARG ECFLOW_GID=1000
RUN { getent group "${ECFLOW_GID}" > /dev/null || groupadd --gid "${ECFLOW_GID}" ecflow; } \
    && useradd --create-home --uid "${ECFLOW_UID}" --gid "${ECFLOW_GID}" ecflow \
    && install -d -o ecflow -g "${ECFLOW_GID}" /workspace

USER ecflow

# Set the working directory
WORKDIR /workspace

EXPOSE 8888

HEALTHCHECK --interval=10s --timeout=8s --start-period=10s \
    CMD ECF_CONNECT_TIMEOUT=5 ecflow_client --http --host localhost --port "${ECFLOW_SERVER_PORT:-8888}" --ping > /dev/null || exit 1

# tini runs as PID 1, forwarding signals to launch.sh and reaping the processes started by the server
ENTRYPOINT ["/usr/bin/tini", "--", "/opt/local/bin/launch.sh"]
