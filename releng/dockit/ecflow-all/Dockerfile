
FROM debian:13.5-slim

RUN apt-get update \
    && apt-get install -y \
        python3 \
        python3-venv \
        libssl-dev \
        libboost-all-dev \
        qt6-base-dev \
        qt6-svg-dev \
        qt6-charts-dev \
        qt6-5compat-dev \
        proxychains4 \
    && rm -rf /var/lib/apt/lists/*

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

ARG ECFLOW_UI_LAUNCH="launch-ecflow_ui.sh"
COPY ${ECFLOW_UI_LAUNCH} /opt/local/bin/${ECFLOW_UI_LAUNCH}
RUN chmod +x /opt/local/bin/${ECFLOW_UI_LAUNCH}

# Set the working directory
WORKDIR /workspace

ENTRYPOINT ["/opt/local/bin/launch-ecflow_ui.sh"]
