#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set -x            # Enable debug mode
set -e            # Exit immediately if a command exits with a non-zero status
set -u            # Treat unset variables as an error when substituting
set -o pipefail   # Return the exit status of the last command in the pipe that failed
set -m            # Enable job control

# Source Python virtual environment

source /opt/local/python/bin/activate

# Define ecflow environment variables (with default values, if not set)

ECFLOW_INSTALL_DIR=${ECFLOW_INSTALL_DIR:-/usr/local}

ECFLOW_WORKSPACE_DIR=${ECFLOW_WORKSPACE_DIR:-/workspace}
ECFLOW_SERVER_HOST=${ECFLOW_SERVER_HOST:-ecflow-server}
ECFLOW_SERVER_PORT=${ECFLOW_SERVER_PORT:-8888}

# Launch ecFlow 'Main' Server

ECF_HOST=${ECFLOW_SERVER_HOST}
export ECF_HOST
ECF_PORT=${ECFLOW_SERVER_PORT}
export ECF_PORT
ECF_HOME=${ECFLOW_WORKSPACE_DIR}
export ECF_HOME
ECF_LOG=${ECF_HOST}.${ECF_PORT}.ecf.log
export ECF_LOG
ECF_CHECK=${ECF_HOST}.${ECF_PORT}.ecf.check
export ECF_CHECK
ECF_CHECKOLD=${ECF_HOST}.${ECF_PORT}.ecf.check.b
export ECF_CHECKOLD
ECF_OUT=${ECF_HOST}.${ECF_PORT}.ecf.out
export ECF_OUT

nohup ${ECFLOW_INSTALL_DIR}/bin/ecflow_server \
      -d \
      --http \
      --port ${ECFLOW_SERVER_PORT} \
      < /dev/null \
      &

wait
