#!/usr/bin/env bash

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
ECFLOW_REST_PORT=${ECFLOW_REST_PORT:-8889}

# Launch ecFlow 'Main' Server

ECF_HOST=${ECFLOW_SERVER_HOST}
ECF_PORT=${ECFLOW_SERVER_PORT}
ECF_HOME=${ECFLOW_WORKSPACE_DIR}
ECF_LOG=${ECF_HOST}.${ECF_PORT}.ecf.log
ECF_CHECK=${ECF_HOST}.${ECF_PORT}.ecf.check
ECF_CHECKOLD=${ECF_HOST}.${ECF_PORT}.ecf.check.b
ECF_OUT=${ECF_HOST}.${ECF_PORT}.ecf.out

nohup ${ECFLOW_INSTALL_DIR}/bin/ecflow_server \
      -d \
      --http \
      --port ${ECFLOW_SERVER_PORT} \
      < /dev/null \
      &

sleep 1 # Give the server a moment to actually start waiting for connections

# Launch ecFlow REST API server

ECF_REST_OUT=${ECF_HOST}.${ECFLOW_REST_PORT}.rest.out

nohup /usr/local/bin/ecflow_http \
      --http \
      --ecflow_host ${ECFLOW_SERVER_HOST} \
      --ecflow_port ${ECFLOW_SERVER_PORT} \
      --port ${ECFLOW_REST_PORT} \
      --no_ssl \
      -v \
      < /dev/null \
      &

wait
