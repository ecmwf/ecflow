#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

#
# Runs a task released by Aviso notifications, end to end: an aviso-server (in a container), an
# ecflow_server, and a suite whose task prints the notification that released it.
#
# Usage: aviso_use_case.sh [/path/to/ecflow/bin]
#
# The directory holding ecflow_server and ecflow_client is taken from the first argument, or from the
# PATH. The following environment variables can be set:
#
#   AVISO_IMAGE   the aviso-server container image (default: eccr.ecmwf.int/aviso/aviso_server:0.13.1)
#   AVISO_PORT    the local port of the aviso-server (default: 8000)
#   ECF_PORT      the port of the ecflow_server (default: 4141)
#

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -ge 1 ]; then
  ECFLOW_BIN="$(cd "$1" && pwd)"
else
  ECFLOW_BIN="$(dirname "$(command -v ecflow_server)")"
fi
AVISO_IMAGE="${AVISO_IMAGE:-eccr.ecmwf.int/aviso/aviso_server:0.13.1}"
AVISO_PORT="${AVISO_PORT:-8000}"
export ECF_HOST=localhost
export ECF_PORT="${ECF_PORT:-4141}"

client="$ECFLOW_BIN/ecflow_client"
work="$(mktemp -d)"
container="ecflow-aviso-use-case-$$"
suite=aviso_use_case
task="/$suite/process"

cleanup() {
  "$client" --terminate=yes >/dev/null 2>&1 || true
  docker stop "$container" >/dev/null 2>&1 || true
  echo "The files of the use case are kept in $work"
}
trap cleanup EXIT

#
# 1. Start the aviso-server
#

echo "Starting the aviso-server ($AVISO_IMAGE) on port $AVISO_PORT"
docker run --rm -d --name "$container" -p "127.0.0.1:$AVISO_PORT:8000" \
  -v "$here/aviso_server_config.yaml:/app/config.yaml:ro" -e AVISOSERVER_CONFIG_FILE=/app/config.yaml \
  "$AVISO_IMAGE" >/dev/null
healthy=false
for _ in $(seq 1 30); do
  if curl -fsS -o /dev/null "http://localhost:$AVISO_PORT/health" 2>/dev/null; then
    healthy=true
    break
  fi
  sleep 1
done
if [ "$healthy" != true ]; then
  echo "The aviso-server did not become healthy within 30 seconds; its log follows" >&2
  docker logs "$container" >&2 || true
  exit 1
fi

#
# 2. Prepare the suite: the credentials file, the task script and the definition
#

# The server has authentication disabled; the credentials file is nevertheless mandatory (here, in the format of
# the ECMWF API credentials file, $HOME/.ecmwfapirc, whose key is sent as a bearer token)
cat >"$work/aviso.json" <<'EOF'
{ "url": "https://api.ecmwf.int/v1", "key": "unused", "email": "user@example.com" }
EOF

mkdir -p "$work/files"
cat >"$work/files/process.ecf" <<'EOF'
#!/usr/bin/env bash
export ECF_HOST=%ECF_HOST% ECF_PORT=%ECF_PORT% ECF_NAME=%ECF_NAME% ECF_PASS=%ECF_PASS% ECF_TRYNO=%ECF_TRYNO% ECF_RID=$$
client=%ECFLOW_BIN%/ecflow_client
trap '$client --abort=trap; exit 1' ERR
$client --init=$$
echo "Released by the Aviso notification:"
echo "  type       = %ECF_AVISO_EVENT_TYPE%"
echo "  sequence   = %ECF_AVISO_EVENT_SEQUENCE%"
echo '  identifier = %ECF_AVISO_EVENT_DATA_IDENTIFIER%'
echo '  payload    = %ECF_AVISO_EVENT_DATA_PAYLOAD%'
$client --complete
EOF

cat >"$work/$suite.def" <<EOF
suite $suite
  edit ECF_HOME '$work'
  edit ECF_FILES '$work/files'
  edit ECFLOW_BIN '$ECFLOW_BIN'
  edit ECF_AVISO_URL 'http://localhost:$AVISO_PORT'
  edit ECF_AVISO_AUTH '$work/aviso.json'
  task process
    aviso --name forecast --listener '{ "event": "mars", "request": { "class": "od", "stream": "enfo", "step": [0, 6, 12] } }'
endsuite
EOF

#
# 3. Start the ecflow_server, and load and begin the suite
#

echo "Starting the ecflow_server on port $ECF_PORT"
(cd "$work" && ECF_HOME="$work" nohup "$ECFLOW_BIN/ecflow_server" --port "$ECF_PORT" >"$work/ecflow_server.out" 2>&1 &)
for _ in $(seq 1 50); do
  "$client" --ping >/dev/null 2>&1 && break
  sleep 0.2
done
"$client" --restart
"$client" --load="$work/$suite.def"
"$client" --begin="$suite"

# The attribute watches for notifications published from the moment its watch is established; the aviso-server
# logs the event api.watch.stream.established when that happens
established=false
for _ in $(seq 1 30); do
  if docker logs "$container" 2>&1 | grep -q '"event.name":"api.watch.stream.established"'; then
    established=true
    break
  fi
  sleep 1
done
if [ "$established" != true ]; then
  echo "The Aviso attribute did not establish its watch within 30 seconds; see $work/ecflow_server.out" >&2
  exit 1
fi

#
# 4. Publish notifications, and follow the releases of the task
#

publish() {
  local step="$1"
  echo "Publishing a mars notification for step $step"
  curl -fsS -X POST "http://localhost:$AVISO_PORT/api/v1/notification" \
    -H 'Content-Type: application/json' \
    -d "{ \"event_type\": \"mars\",
          \"identifier\": { \"class\": \"od\", \"stream\": \"enfo\", \"step\": $step },
          \"payload\": { \"location\": \"file:///data/od/enfo/step$step.grib\" } }" >/dev/null
}

wait_until_complete() {
  for _ in $(seq 1 60); do
    [ "$("$client" --query state "$task")" = "complete" ] && return 0
    sleep 1
  done
  echo "The task $task was not released" >&2
  return 1
}

# Step 3 is not selected by the listener, and does not release the task
publish 3
publish 0
publish 6

# Each notification releases the task once, in order: step 0, then step 6 after the requeue
for release in 1 2; do
  wait_until_complete
  echo "Release $release of $task:"
  grep -E '^ ' "$work/$suite/process.1"
  "$client" --requeue=force "$task"
done
