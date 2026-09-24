#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set -e            # Exit immediately if a command exits with a non-zero status
set -u            # Treat unset variables as an error when substituting
set -o pipefail   # Return the exit status of the last command in the pipe that failed

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

# Stop the server cleanly on SIGTERM/SIGINT (e.g. `docker stop`), as ecflow_server does not act on
# SIGTERM: request termination through the client, and kill the server only if that request fails

server_pid=""

# shellcheck disable=SC2329 # invoked through the trap below
function terminate() {
    echo "launch.sh: stopping ecflow_server on port ${ECFLOW_SERVER_PORT}" >&2
    if ! ECF_CONNECT_TIMEOUT=5 "${ECFLOW_INSTALL_DIR}/bin/ecflow_client" \
            --http --host localhost --port "${ECFLOW_SERVER_PORT}" --terminate=yes; then
        echo "launch.sh: terminate request failed; killing ecflow_server" >&2
        kill -KILL "${server_pid}" 2>/dev/null || true
    fi
}

trap terminate TERM INT

# Any arguments given to the container are passed on to the server (e.g. -d, for debug output)

"${ECFLOW_INSTALL_DIR}/bin/ecflow_server" \
      --http \
      --port "${ECFLOW_SERVER_PORT}" \
      "$@" \
      < /dev/null \
      &
server_pid=$!

# 'wait' returns early when a trapped signal arrives; keep waiting until the server itself has
# exited, and exit with its status

set +e
wait "${server_pid}"
status=$?
while kill -0 "${server_pid}" 2>/dev/null; do
    wait "${server_pid}"
    status=$?
done

exit "${status}"
