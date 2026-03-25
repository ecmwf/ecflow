
FROM debian:12.11-slim AS base

RUN apt-get update \
    && apt-get install -y \
        python3 \
        python3-venv \
        libssl-dev \
        libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /workspace


FROM base AS builder

RUN apt-get update \
    && apt-get install -y \
        gcc \
        g++ \
        cmake \
        ninja-build \
        qt6-base-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy build workflow script
COPY builder/workspace/build.workflow.sh /workspace/build.workflow.sh
# Make the build workflow script executable
RUN chmod +x build.workflow.sh
# Set the entrypoint to the build script
ENTRYPOINT ["/workspace/build.workflow.sh"]


FROM base AS deploy

# Provision ecFlow dependencies
RUN python3 -m venv /opt/local/python \
    && /opt/local/python/bin/pip install "troika>=0.2.6"

# Provision ecFlow package
## This Dockerfile assumes that the ecFlow package is available in the build context i.e. .../imachination/ecflow.
# If you are using a different version, change the ECFLOW_PACKAGE variable accordingly.
ARG ECFLOW_VERSION="latest"
ARG ECFLOW_PACKAGE="ecflow-${ECFLOW_VERSION}-Linux.deb"
COPY ${ECFLOW_PACKAGE} /tmp/${ECFLOW_PACKAGE}
RUN dpkg -i /tmp/${ECFLOW_PACKAGE} \
    && rm /tmp/${ECFLOW_PACKAGE}

ARG ECFLOW_LAUNCH="launch.sh"
COPY ${ECFLOW_LAUNCH} /opt/local/bin/${ECFLOW_LAUNCH}
RUN chmod +x /opt/local/bin/${ECFLOW_LAUNCH}

ENTRYPOINT ["/opt/local/bin/launch.sh"]
