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
readonly CLUSTER_CONFIG="${HERE}/kind-cluster.yaml"

# The administrator's files that every deployment starts from (server_environment.cfg); `admin <dir>`
# merges a directory of per-user files over them.
readonly ADMIN_BASE_DIR="${IMACHINATION_DIR}/ecflow/admin"

# The images named in the manifests, and those used in their place: the
# *_SOURCE variables select another image (a branch build, or one built
# locally), which is loaded into the cluster under its own name, and replaces
# the image of the manifests when the stack is applied. The node never pulls an
# image: the manifests use IfNotPresent, and every image is loaded beforehand.
readonly ECFLOW_DEFAULT="eccr.ecmwf.int/ecflow-dev-environments/ecflow-server-dev:latest"
readonly AUTHOTRON_DEFAULT="eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.3.7"
readonly REVPROXY_DEFAULT="eccr.ecmwf.int/ecflow-dev-environments/ecflow-revproxy-dev:latest"
readonly SFTP_DEFAULT="eccr.ecmwf.int/ecflow-dev-environments/ecflow-sftp-dev:latest"
readonly ECFLOW_SOURCE="${ECFLOW_SOURCE:-${ECFLOW_DEFAULT}}"
readonly AUTHOTRON_SOURCE="${AUTHOTRON_SOURCE:-${AUTHOTRON_DEFAULT}}"
readonly REVPROXY_SOURCE="${REVPROXY_SOURCE:-${REVPROXY_DEFAULT}}"
readonly SFTP_SOURCE="${SFTP_SOURCE:-${SFTP_DEFAULT}}"

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

do_cluster() {
    require kind kubectl docker

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
# so that a tag republished upstream (such as `latest`) reaches the cluster; an
# image that cannot be pulled, such as one built locally, is used as cached.
ensure_pulled() {
    local image="$1" cached=0
    docker image inspect "${image}" >/dev/null 2>&1 && cached=1
    if [[ "${REFRESH_IMAGES:-0}" != 1 && "${cached}" == 1 ]]; then
        info "Already cached: ${image}"
        return
    fi
    info "Pulling ${image}"
    if ! docker pull "${image}"; then
        [[ "${cached}" == 1 ]] || die "Failed to pull ${image}. A 'docker login eccr.ecmwf.int' may be required."
        warn "Failed to pull ${image}; using the cached image."
    fi
}

do_images() {
    require kind docker
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."

    # Every image is published, for both linux/amd64 and linux/arm64, so the
    # variant matching the host is pulled and no emulation is involved. The
    # images of dockit (the ecFlow server, the reverse proxy and the SFTP
    # sidecar) can also be built locally, from releng/dockit, and named here
    # through the *_SOURCE variables; an image already cached is not pulled.
    local image
    for image in "${ECFLOW_SOURCE}" "${AUTHOTRON_SOURCE}" "${REVPROXY_SOURCE}" "${SFTP_SOURCE}"; do
        ensure_pulled "${image}"
        info "Loading ${image} into the cluster"
        kind load docker-image "${image}" --name "${CLUSTER_NAME}"
    done
}

# Writes, to standard output, the objects of the stack, with the images of the
# manifests replaced by those selected by the *_SOURCE variables. Each image of
# the manifests is matched exactly, as the whole value of an `image:` field,
# which may open an item of a list (`- image:`).
render_stack() {
    local sed_args=() pair default source
    for pair in "${ECFLOW_DEFAULT}=${ECFLOW_SOURCE}" "${AUTHOTRON_DEFAULT}=${AUTHOTRON_SOURCE}" \
                "${REVPROXY_DEFAULT}=${REVPROXY_SOURCE}" "${SFTP_DEFAULT}=${SFTP_SOURCE}"; do
        default="${pair%%=*}"
        source="${pair#*=}"
        sed_args+=(-e "s|^\([ -]*image: \)${default//./\\.}\$|\1${source}|")
    done
    kubectl kustomize "${IMACHINATION_DIR}" | sed "${sed_args[@]}"
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
ensure_namespace() {
    kubectl --context "kind-${CLUSTER_NAME}" create namespace "${NAMESPACE}" --dry-run=client -o yaml \
        | kubectl --context "kind-${CLUSTER_NAME}" apply -f - >/dev/null
}

# Writes, to standard output, a Secret named $1 holding every file below the directory $2, each keyed by
# its path relative to $2, with "__" in place of "/"; files below the directories named in $3 (a
# space-separated list) are left out.
secret_from_tree() {
    local name="$1" dir="$2" skip="${3:-}" file relative key
    local args=()
    while IFS= read -r file; do
        relative="${file#"${dir}"/}"
        local skipped=0 top
        for top in ${skip}; do
            [[ "${relative}" == "${top}/"* ]] && skipped=1
        done
        (( skipped )) && continue
        key="${relative//\//__}"
        args+=("--from-file=${key}=${file}")
    done < <(find "${dir}" -type f ! -name '.*' | sort)
    kubectl --context "kind-${CLUSTER_NAME}" create secret generic "${name}" -n "${NAMESPACE}" \
        --dry-run=client -o yaml ${args[@]+"${args[@]}"}
}

# Loads the administrator's files into the cluster: ecflow-admin, which the ecFlow server Pod expands into
# /admin, holds the base files merged with those of the given directory (server_environment.cfg, troikaw,
# troika/<user>/, secrets/<user>/, ...); sftp-keys, which only the SFTP sidecar mounts, holds the files of
# its sshd/ directory (<user>.authorized_keys and the host key). The server Pod is then replaced, so that
# it expands the new files.
do_admin() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."
    local dir="${1:-}"
    if [[ -n "${dir}" ]]; then
        [[ -d "${dir}" ]] || die "No such directory: ${dir}"
        dir="$(cd "${dir}" && pwd)"
    fi

    local staging
    staging="$(mktemp -d)"
    # shellcheck disable=SC2064 # the directory is known now, and must be removed on any exit
    trap "rm -rf '${staging}'" EXIT
    cp -R "${ADMIN_BASE_DIR}/." "${staging}/"
    if [[ -n "${dir}" ]]; then
        cp -R "${dir}/." "${staging}/"
    fi

    ensure_namespace
    info "Loading the administrator's files${dir:+ from ${dir}} into Secret ecflow-admin"
    secret_from_tree ecflow-admin "${staging}" "sshd venv" \
        | kubectl --context "kind-${CLUSTER_NAME}" apply -f -
    if [[ -d "${staging}/sshd" ]]; then
        info "Loading the SSH keys into Secret sftp-keys"
        secret_from_tree sftp-keys "${staging}/sshd" \
            | kubectl --context "kind-${CLUSTER_NAME}" apply -f -
    fi

    if kubectl --context "kind-${CLUSTER_NAME}" get deployment/ecflow-server -n "${NAMESPACE}" >/dev/null 2>&1; then
        do_restart ecflow-server
        warn "The ecFlow server restarted halted: issue 'ecflow_client --https --restart' as an administrator."
    fi
}

do_apply() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."

    # The server Pod cannot start without the administrator's files; the base is loaded when none are,
    # and files already loaded are kept
    ensure_namespace
    if ! kubectl --context "kind-${CLUSTER_NAME}" get secret ecflow-admin -n "${NAMESPACE}" >/dev/null 2>&1; then
        info "Loading the base administrator's files into Secret ecflow-admin"
        secret_from_tree ecflow-admin "${ADMIN_BASE_DIR}" \
            | kubectl --context "kind-${CLUSTER_NAME}" apply -f -
    fi

    info "Applying the stack"
    render_stack | kubectl --context "kind-${CLUSTER_NAME}" apply -f -

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

# Refreshes the images and replaces the workloads that use them: the images are
# pulled again even when cached, the stack is applied again, so that the images
# selected by the *_SOURCE variables replace those in use, and every workload,
# or the one named, is restarted onto the images just loaded.
do_reload() {
    REFRESH_IMAGES=1 do_images
    do_apply
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
    # readiness probe, ecflow_ui) write there meanwhile. The log is in the
    # workspace, on a volume of the cluster, and is searched within the Pod.
    local marker="/imachination-verify-$$-${RANDOM}"
    local log="/workspace/ecflow-server.8888.ecf.log"

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
    # grep exits with 0 when the marker is found, 1 when it is not, and 2 when
    # the log cannot be read; kubectl exec passes the status on
    local count status=0
    count="$(kubectl --context "kind-${CLUSTER_NAME}" exec deployment/ecflow-server -n "${NAMESPACE}" \
        -c ecflow-server -- grep -c -e "${marker}" "${log}" 2>/dev/null)" || status=$?
    case "${status}" in
        0) warn "FAIL  a refused request reached the server: ${count} line(s) in its log"
           failed=1 ;;
        1) info "PASS  refused requests leave the server log untouched" ;;
        *) warn "SKIP  server log not readable in the ecFlow server Pod: ${log}" ;;
    esac

    [[ "${failed}" -eq 0 ]] || die "The authenticated path does not behave as expected."
    info "The authenticated path behaves as expected."
}

# Removes the stack, leaving the cluster and its loaded images in place. The
# volumes of the stack go with it: the workspace and the checkpoint are lost.
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
        | grep -E 'IMAGE|ecflow-dev-environments|auth-o-tron' || warn "No images of the stack loaded yet."

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
}

usage() {
    cat <<USAGE
Usage: $(basename "$0") <command> [argument]

Commands:
  up         Create the cluster, load the images and apply the stack, in one go
  cluster    Create the cluster, if absent
  images     Pull, tag and load the container images into the cluster
  apply      Declare the stack in the cluster and wait for it to become available
  admin DIR  Load the administrator's files (DIR merged over ecflow/admin) and the
             SSH keys (DIR/sshd), then replace the ecFlow server Pod
  verify     Exercise the authenticated path with ecflow_client, from the host
  status     Report the state of the cluster, its images and the stack
  logs       Follow the output of every workload, or of the one named
  restart    Replace every workload, or the one named, so that it re-reads its
             configuration; required after editing server_environment.cfg
  reload     Pull the images again, even when cached, load them, apply the
             stack, and restart every workload, or the one named, onto them
  down       Delete the stack, with its workspace and checkpoint, keeping the
             cluster and its images
  destroy    Delete the cluster

Environment:
  ECFLOW_SOURCE      Source image for the ecFlow server, the equivalent of the
                     ECFLOW_IMAGE that compose.yaml accepts
  AUTHOTRON_SOURCE   Source image for the authentication service
  REVPROXY_SOURCE    Source image for the reverse proxy, the equivalent of the
                     REVPROXY_IMAGE that compose.yaml accepts
  SFTP_SOURCE        Source image for the SFTP sidecar of the ecFlow server
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
        admin)   do_admin "${2:-}" ;;
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
