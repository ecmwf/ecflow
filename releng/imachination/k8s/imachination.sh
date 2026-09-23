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
ensure_pulled() {
    local image="$1"
    if docker image inspect "${image}" >/dev/null 2>&1; then
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

# Declares every object of the stack. Re-applying an unchanged tree is a no-op,
# and an edited configuration file yields a new generated name, which rolls the
# pods that consume it.
do_apply() {
    require kubectl
    cluster_exists || die "Cluster '${CLUSTER_NAME}' does not exist. Run '$(basename "$0") cluster' first."

    info "Applying the stack"
    kubectl --context "kind-${CLUSTER_NAME}" apply -k "${IMACHINATION_DIR}"
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
Usage: $(basename "$0") <command>

Commands:
  cluster    Render the cluster definition and create the cluster, if absent
  images     Build, pull, tag and load the container images into the cluster
  apply      Declare the stack in the cluster
  down       Delete the stack, keeping the cluster and its images
  status     Report the state of the cluster, its images and the stack
  destroy    Delete the cluster and the rendered definition

Environment:
  WORKSPACE_DIR      Workspace mounted into the cluster
                     (default: <imachination>/ecflow/workspace)
  ECFLOW_SOURCE      Source image for the ecFlow server, the equivalent of the
                     ECFLOW_IMAGE that compose.yaml accepts
  AUTHOTRON_SOURCE   Source image for the authentication service
USAGE
}

main() {
    case "${1:-}" in
        cluster) do_cluster ;;
        images)  do_images ;;
        apply)   do_apply ;;
        down)    do_down ;;
        status)  do_status ;;
        destroy) do_destroy ;;
        ""|-h|--help|help) usage ;;
        *) usage; die "Unknown command: $1" ;;
    esac
}

main "$@"
