#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Drives the `imachination` stack on a local Kubernetes cluster, providing the
# equivalent of `docker compose up` for the kind-based deployment.
#
# Every action is idempotent: running a subcommand a second time either does
# nothing or converges on the same state.

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly HERE
IMACHINATION_DIR="$(cd "${HERE}/.." && pwd)"
readonly IMACHINATION_DIR

readonly CLUSTER_NAME="imachination"
readonly NAMESPACE="imachination"
readonly GENERATED_DIR="${HERE}/.generated"
readonly CLUSTER_TEMPLATE="${HERE}/kind-cluster.yaml.in"
readonly CLUSTER_CONFIG="${GENERATED_DIR}/kind-cluster.yaml"

# The workspace mounted into the cluster, overridable in the same way that
# compose.yaml allows.
WORKSPACE_DIR="${WORKSPACE_DIR:-${IMACHINATION_DIR}/ecflow/workspace}"

# Images are renamed on the way into the cluster. A tag other than `latest` is
# used deliberately: Kubernetes defaults imagePullPolicy to Always for `latest`,
# which would send the cluster to a registry it has no credentials for, instead
# of using the copy loaded here.
readonly ECFLOW_SOURCE="${ECFLOW_SOURCE:-eccr.ecmwf.int/ecflow-dev-environments/ecflow-server-dev:latest}"
readonly ECFLOW_IMAGE="imachination/ecflow-server:local"
readonly AUTHOTRON_SOURCE="${AUTHOTRON_SOURCE:-eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.3.7}"
readonly AUTHOTRON_IMAGE="imachination/authotron:local"
readonly REVPROXY_IMAGE="imachination/revproxy:local"

info() { printf '\033[1m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[33m==> %s\033[0m\n' "$*" >&2; }
die()  { printf '\033[31m==> %s\033[0m\n' "$*" >&2; exit 1; }

require() {
    for tool in "$@"; do
        command -v "${tool}" >/dev/null 2>&1 || die "'${tool}' is required, but is not installed."
    done
}

cluster_exists() {
    kind get clusters 2>/dev/null | grep -qx "${CLUSTER_NAME}"
}

# Renders the cluster definition, substituting the absolute workspace path that
# extraMounts requires and a relative path cannot supply.
render_cluster_config() {
    [[ -d "${WORKSPACE_DIR}" ]] || die "Workspace directory does not exist: ${WORKSPACE_DIR}"
    local resolved
    resolved="$(cd "${WORKSPACE_DIR}" && pwd)"

    mkdir -p "${GENERATED_DIR}"
    sed "s|__WORKSPACE_DIR__|${resolved}|" "${CLUSTER_TEMPLATE}" > "${CLUSTER_CONFIG}"
    info "Rendered ${CLUSTER_CONFIG} (workspace: ${resolved})"
}

do_cluster() {
    require kind kubectl docker
    render_cluster_config

    if cluster_exists; then
        info "Cluster '${CLUSTER_NAME}' already exists; leaving it alone."
    else
        info "Creating cluster '${CLUSTER_NAME}'"
        kind create cluster --config "${CLUSTER_CONFIG}"
    fi

    kubectl --context "kind-${CLUSTER_NAME}" wait --for=condition=Ready nodes --all --timeout=180s
}

# Ensures an image is in the host Docker cache, pulling it only when absent so
# that a working cluster does not depend on registry availability.
#
# With REFRESH_IMAGES=1, as set by `reload`, the image is pulled even when cached,
# so that a tag republished upstream (such as `latest`) reaches the cluster.
ensure_pulled() {
    local image="$1"
    if [[ "${REFRESH_IMAGES:-0}" != 1 ]] && docker image inspect "${image}" >/dev/null 2>&1; then
        info "Already cached: ${image}"
        return
    fi
    info "Pulling ${image}"
    docker pull "${image}" \
        || die "Failed to pull ${image}. A 'docker login eccr.ecmwf.int' may be required."
}

do_images() {
    require kind docker
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."

    # The reverse proxy has no published image and is always built locally.
    info "Building ${REVPROXY_IMAGE}"
    docker build -t "${REVPROXY_IMAGE}" "${IMACHINATION_DIR}/revproxy"

    # The ecFlow image is published for both linux/amd64 and linux/arm64, so the
    # variant matching the host is pulled and no emulation is involved.
    ensure_pulled "${ECFLOW_SOURCE}"
    docker tag "${ECFLOW_SOURCE}" "${ECFLOW_IMAGE}"

    ensure_pulled "${AUTHOTRON_SOURCE}"
    docker tag "${AUTHOTRON_SOURCE}" "${AUTHOTRON_IMAGE}"

    for image in "${ECFLOW_IMAGE}" "${AUTHOTRON_IMAGE}" "${REVPROXY_IMAGE}"; do
        info "Loading ${image} into the cluster"
        kind load docker-image "${image}" --name "${CLUSTER_NAME}"
    done
}

# Reports why a workload has not converged. Called only on failure, when the
# cause is invariably in the pod's own events or in what it last printed.
diagnose() {
    local workload="$1"
    warn "Diagnosis for ${workload}"
    kubectl --context "kind-${CLUSTER_NAME}" get pods -n "${NAMESPACE}" 2>&1 || true
    warn "Recent events"
    kubectl --context "kind-${CLUSTER_NAME}" get events -n "${NAMESPACE}" \
        --sort-by=.lastTimestamp 2>&1 | tail -10 || true
    warn "Last output of ${workload}"
    kubectl --context "kind-${CLUSTER_NAME}" logs "${workload}" -n "${NAMESPACE}" \
        --tail=20 --all-containers 2>&1 | tail -20 || true
}

# Waits for the named deployments to become available, or for every declared one
# when none is named, so that a workload which fails to start is reported here
# rather than discovered later. Only what an action touched is waited for, so
# that the report names the workloads that actually moved.
wait_for_workloads() {
    local timeout="${ROLLOUT_TIMEOUT:-300s}"
    local workload
    local failed=0

    # `kubectl get -o name` rather than an array, as this script must run under
    # the bash 3.2 that macOS provides, where `mapfile` is unavailable.
    while IFS= read -r workload; do
        [[ -n "${workload}" ]] || continue
        info "Waiting for ${workload} (timeout ${timeout})"
        if ! kubectl --context "kind-${CLUSTER_NAME}" rollout status "${workload}" \
                -n "${NAMESPACE}" --timeout="${timeout}"; then
            diagnose "${workload}"
            failed=1
        fi
    done < <(if [[ "$#" -gt 0 ]]; then
                 printf 'deployment.apps/%s\n' "$@"
             else
                 kubectl --context "kind-${CLUSTER_NAME}" get deployments \
                     -n "${NAMESPACE}" -o name 2>/dev/null
             fi)

    [[ "${failed}" -eq 0 ]] || die "One or more workloads did not become available."
}

# Declares every object of the stack. Re-applying an unchanged tree is a no-op,
# and an edited configuration file yields a new generated name, which rolls the
# pods that consume it.
do_apply() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."

    info "Applying the stack"
    kubectl --context "kind-${CLUSTER_NAME}" apply -k "${IMACHINATION_DIR}"

    wait_for_workloads
    info "The stack is available."
}

# Restarts workloads so that they re-read their configuration. The ecFlow server
# reads server_environment.cfg only at start-up, and offers no command to reload
# ECF_PERMISSIONS, so editing that file has no effect until the pod is replaced.
do_restart() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist."

    local target="${1:-}"
    if [[ -n "${target}" ]]; then
        kubectl --context "kind-${CLUSTER_NAME}" get "deployment/${target}" \
            -n "${NAMESPACE}" >/dev/null 2>&1 \
            || die "No such workload: ${target}. Try '$(basename "$0") status'."
        info "Restarting deployment/${target}"
        kubectl --context "kind-${CLUSTER_NAME}" rollout restart \
            "deployment/${target}" -n "${NAMESPACE}"
    else
        info "Restarting every workload"
        kubectl --context "kind-${CLUSTER_NAME}" rollout restart deployment -n "${NAMESPACE}"
    fi

    # Only the workload that was restarted is waited for, so that the report does
    # not name workloads that were left running.
    wait_for_workloads ${target:+"${target}"}
    info "The stack is available."
}

# Brings the whole stack up from nothing: the cluster, the images and the
# workloads. Each step is idempotent, so running it again on a working stack
# changes nothing.
do_up() {
    do_cluster
    do_images
    do_apply
}

# Refreshes the images and replaces the workloads that use them: the reverse
# proxy is rebuilt, the published images are pulled again even when cached, and
# every workload, or the one named, is restarted onto the images just loaded.
do_reload() {
    REFRESH_IMAGES=1 do_images
    do_restart "${1:-}"
}

# Follows the output of one workload, or of every workload with each line
# prefixed by the pod it came from.
do_logs() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist."

    local target="${1:-}"
    if [[ -n "${target}" ]]; then
        kubectl --context "kind-${CLUSTER_NAME}" logs -f "deployment/${target}" \
            -n "${NAMESPACE}" --all-containers --tail=50
    else
        kubectl --context "kind-${CLUSTER_NAME}" logs -f -n "${NAMESPACE}" \
            -l 'app in (ecflow-server, authotron, revproxy)' \
            --all-containers --prefix --tail=20 --max-log-requests=6
    fi
}

# Exercises the authenticated path end to end with the real client, from the
# host: valid credentials reach the server, while missing and wrong credentials
# are refused by the reverse proxy and leave the server log untouched.
do_verify() {
    require ecflow_client
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist."

    local user="${VERIFY_USER:-admin}"
    local password="${VERIFY_PASSWORD:-somesecret#admin}"
    local server="https://localhost:443"
    local scratch
    scratch="$(mktemp -d)"
    # shellcheck disable=SC2064 # the directory is known now, and must be removed on any exit
    trap "rm -rf '${scratch}'" EXIT

    tokens() {
        printf '{"version": 1, "tokens": [%s]}\n' "$1"
    }
    basic() {
        printf '{"type": "basic", "server": "%s", "api": {"username": "%s", "password": "%s"}}' \
            "${server}" "$1" "$2"
    }
    ( umask 077
      tokens "$(basic "${user}" "${password}")" > "${scratch}/valid"
      tokens "$(basic "${user}" "${password}-wrong")" > "${scratch}/wrong"
      tokens "" > "${scratch}/none" )

    # The refused requests query a node path unique to this run, so that any of
    # them reaching the server is found in its log whatever other clients (the
    # readiness probe, ecflow_ui) write there meanwhile.
    local marker="/imachination-verify-$$-${RANDOM}"
    local log="${WORKSPACE_DIR}/ecflow-server.8888.ecf.log"

    local failed=0
    client_as() {
        local tokens="$1"
        shift
        ECF_AUTHTOKENS="${tokens}" ECF_HOST=localhost ECF_PORT=443 \
            ecflow_client --https "$@" 2>&1
    }
    expect() {
        local what="$1" want="$2" output="$3"
        if grep -q "${want}" <<<"${output}"; then
            info "PASS  ${what}"
        else
            warn "FAIL  ${what}: $(head -n 2 <<<"${output}" | tr '\n' ' ')"
            failed=1
        fi
    }

    expect "valid credentials (${user}) reach the server" "succeeded" \
        "$(client_as "${scratch}/valid" --ping)"
    expect "no credentials are refused" "Unauthorized (401)" \
        "$(client_as "${scratch}/none" --query state "${marker}")"
    expect "a wrong password is refused" "Unauthorized (401)" \
        "$(client_as "${scratch}/wrong" --query state "${marker}")"
    sleep 1
    if [[ ! -f "${log}" ]]; then
        warn "SKIP  server log not found in the workspace: ${log}"
    elif grep -q -- "${marker}" "${log}"; then
        warn "FAIL  a refused request reached the server: $(grep -c -- "${marker}" "${log}") line(s) in its log"
        failed=1
    else
        info "PASS  refused requests leave the server log untouched"
    fi

    [[ "${failed}" -eq 0 ]] || die "The authenticated path does not behave as expected."
    info "The authenticated path behaves as expected."
}

# Removes the stack, leaving the cluster and its loaded images in place.
do_down() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist."

    info "Deleting namespace '${NAMESPACE}'"
    kubectl --context "kind-${CLUSTER_NAME}" delete namespace "${NAMESPACE}" --ignore-not-found
}

do_status() {
    require kubectl
    if ! cluster_exists; then
        warn "Cluster '${CLUSTER_NAME}' does not exist."
        return
    fi
    info "Nodes"
    kubectl --context "kind-${CLUSTER_NAME}" get nodes
    info "Images loaded into the cluster"
    docker exec "${CLUSTER_NAME}-control-plane" crictl images 2>/dev/null \
        | grep -E 'IMAGE|imachination' || warn "No imachination images loaded yet."

    info "Objects in namespace '${NAMESPACE}'"
    kubectl --context "kind-${CLUSTER_NAME}" get all,configmap,secret -n "${NAMESPACE}" 2>/dev/null \
        | grep -v '^configmap/kube-root-ca.crt' || warn "Namespace '${NAMESPACE}' does not exist yet."
}

do_destroy() {
    require kind
    if cluster_exists; then
        info "Deleting cluster '${CLUSTER_NAME}'"
        kind delete cluster --name "${CLUSTER_NAME}"
    else
        info "Cluster '${CLUSTER_NAME}' does not exist; nothing to delete."
    fi
    rm -rf "${GENERATED_DIR}"
}

usage() {
    cat <<USAGE
Usage: $(basename "$0") <command> [argument]

Commands:
  up         Create the cluster, load the images and apply the stack, in one go
  cluster    Render the cluster definition and create the cluster, if absent
  images     Build, pull, tag and load the container images into the cluster
  apply      Declare the stack in the cluster and wait for it to become available
  verify     Exercise the authenticated path with ecflow_client, from the host
  status     Report the state of the cluster, its images and the stack
  logs       Follow the output of every workload, or of the one named
  restart    Replace every workload, or the one named, so that it re-reads its
             configuration; required after editing server_environment.cfg
  reload     Rebuild and pull the images again, even when cached, load them, and
             restart every workload, or the one named, onto them
  down       Delete the stack, keeping the cluster and its images
  destroy    Delete the cluster and the rendered definition

Environment:
  WORKSPACE_DIR      Workspace mounted into the cluster
                     (default: <imachination>/ecflow/workspace)
  ECFLOW_SOURCE      Source image for the ecFlow server, the equivalent of the
                     ECFLOW_IMAGE that compose.yaml accepts
  AUTHOTRON_SOURCE   Source image for the authentication service
  ROLLOUT_TIMEOUT    How long to wait for a workload to become available
                     (default: 300s; an emulated server starts slowly)
  VERIFY_USER        User of the plain provider that verify authenticates as
  VERIFY_PASSWORD    Its password (default: the test user admin, as defined in
                     authotron/config.yaml)
USAGE
}

main() {
    case "${1:-}" in
        up)      do_up ;;
        cluster) do_cluster ;;
        images)  do_images ;;
        apply)   do_apply ;;
        verify)  do_verify ;;
        status)  do_status ;;
        logs)    do_logs "${2:-}" ;;
        restart) do_restart "${2:-}" ;;
        reload)  do_reload "${2:-}" ;;
        down)    do_down ;;
        destroy) do_destroy ;;
        ""|-h|--help|help) usage ;;
        *) usage; die "Unknown command: $1" ;;
    esac
}

main "$@"
