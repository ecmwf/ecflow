# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

FROM debian:13.7-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        python3-venv \
        tini \
    && rm -rf /var/lib/apt/lists/*

# Provision ecFlow dependencies
# The virtual environment includes the system site packages, so that the ecflow Python module installed by the
# package below is available in it, alongside troika
RUN python3 -m venv --system-site-packages /opt/local/python \
    && /opt/local/python/bin/pip install "troika>=0.2.6"

# Provision ecFlow package
# This Dockerfile assumes that the ecFlow package for each target platform is available in the build context
# (i.e. this directory), named after the platform architecture: ecflow-amd64.deb, ecflow-arm64.deb.
# TARGETARCH, set by BuildKit, selects the package, so that a single multi-platform build installs the
# matching package on each platform.
# The package is bind-mounted, rather than copied, so that it is not kept in an image layer, and installed
# with apt-get, so that the runtime libraries it depends on are installed as well.
ARG TARGETARCH
RUN --mount=type=bind,source=ecflow-${TARGETARCH}.deb,target=/tmp/ecflow.deb \
    apt-get update \
    && apt-get install -y --no-install-recommends /tmp/ecflow.deb \
    && rm -rf /var/lib/apt/lists/*

ARG ECFLOW_LAUNCH="launch.sh"
COPY ${ECFLOW_LAUNCH} /opt/local/bin/${ECFLOW_LAUNCH}
RUN chmod +x /opt/local/bin/${ECFLOW_LAUNCH}

# Run as an unprivileged user
# The server runs as the user 'ecflow'. The container starts as root, and launch.sh aligns the UID/GID of
# 'ecflow' with the owner of the workspace before dropping privileges to it, so that a bind-mounted workspace
# is writable whatever UID owns it on the host. ECFLOW_UID/ECFLOW_GID only set the initial UID/GID, used
# when the workspace is not mounted from the host. ecFlow identifies users by their passwd entry, so the
# container must not run under an arbitrary `docker run --user` UID. The GID may duplicate that of an
# existing group (e.g. GID 20, the default group on macOS).
ARG ECFLOW_UID=1000
ARG ECFLOW_GID=1000
RUN groupadd --non-unique --gid "${ECFLOW_GID}" ecflow \
    && useradd --create-home --uid "${ECFLOW_UID}" --gid "${ECFLOW_GID}" ecflow \
    && install -d -o ecflow -g ecflow /workspace

# Set the working directory
WORKDIR /workspace

EXPOSE 8888

# The health check runs as root; the ping is issued as 'ecflow', the user the server runs as, with its own
# HOME (ecflow_client fails, reporting a connection error, when HOME is not accessible to it)
HEALTHCHECK --interval=10s --timeout=8s --start-period=10s \
    CMD HOME=/home/ecflow ECF_CONNECT_TIMEOUT=5 setpriv --reuid=ecflow --regid=ecflow --init-groups \
        ecflow_client --http --host localhost --port "${ECFLOW_SERVER_PORT:-8888}" --ping > /dev/null || exit 1

# tini runs as PID 1, forwarding signals to launch.sh and reaping the processes started by the server
ENTRYPOINT ["/usr/bin/tini", "--", "/opt/local/bin/launch.sh"]
