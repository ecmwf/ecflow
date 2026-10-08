<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# ecflow-chart

A Helm chart that deploys an ecFlow server on Kubernetes: the server with its SFTP sidecar,
behind an nginx reverse proxy that authenticates every request with
[auth-o-tron](https://github.com/ecmwf/auth-o-tron). auth-o-tron itself is deployed by
[auth-o-tron-chart](https://github.com/ecmwf/auth-o-tron-chart), a dependency of this chart.

## Layout

| Path | Content |
|---|---|
| `Chart.yaml` | Name, version (the ecFlow version) and dependencies of the chart. |
| `values.yaml` | The default values; a deployment overrides them with a profile. |
| `templates/` | The Kubernetes objects, rendered from the values. |
| `templates/_helpers.tpl` | Named templates (names and labels) shared by the templates. |
| `templates/NOTES.txt` | The text printed after `helm install` and `helm upgrade`. |
| `charts/` | The packaged dependencies, fetched by `helm dependency update` (not committed). |
| `Chart.lock` | The exact versions of the dependencies fetched. |
| `examples/` | Value profiles of the known targets. |

## Prerequisites

- Helm 3.8 or later (OCI registries), and access to `eccr.ecmwf.int`, from which the
  dependency chart and the images are pulled.
- A Kubernetes cluster, or [kind](https://kind.sigs.k8s.io/) for a local deployment.

## Usage

Fetch the dependencies, then check the chart and render it without a cluster:

```bash
cd releng/ecflow-chart
helm dependency update
helm lint
helm template ecflow . > /dev/null
```

Install, or upgrade, a release named `ecflow` in a namespace, with a profile:

```bash
helm upgrade --install ecflow . -n <namespace> --create-namespace -f examples/values-<target>.yaml
```

| Profile | Target |
|---|---|
| `examples/values-kind.yaml` | A local kind cluster with the community ingress-nginx controller. |
| `examples/values-webapps-test.yaml` | The ECMWF cluster webapps-test, namespace `ecflow-multitenant-test`, NGINX Inc controller. |

The accounts of a deployment are the `users` list of its profile: each user with SSH
keys gets an SFTP account. The test users of auth-o-tron in `values.yaml` are for test
deployments only.

## A local deployment on kind

The recipe below creates a throwaway cluster, installs the community ingress-nginx
controller, loads the images, deploys the chart with the kind profile and checks the
result from the host. It needs Docker, [kind](https://kind.sigs.k8s.io/), `kubectl`,
`helm`, a login to `eccr.ecmwf.int` (`docker login eccr.ecmwf.int`), and an
`ecflow_client` built with SSL support. Run it from this directory.

```bash
# 1. The cluster, with the controller's ports mapped to the host, and the controller.
kind create cluster --config examples/kind-cluster.yaml
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-v1.13.0/deploy/static/provider/kind/deploy.yaml
kubectl -n ingress-nginx wait deploy/ingress-nginx-controller --for=condition=Available --timeout=180s

# 2. The images, pulled with the registry login and loaded into the node.
IMAGES="eccr.ecmwf.int/ecflow-dev-environments/ecflow-server-dev:latest
        eccr.ecmwf.int/ecflow-dev-environments/ecflow-sftp-dev:latest
        eccr.ecmwf.int/ecflow-dev-environments/ecflow-revproxy-dev:latest
        eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.3.7"
for image in $IMAGES; do docker pull "$image"; done
kind load docker-image --name ecflow $IMAGES

# 3. The release, with the kind profile and the SFTP users (a values file with the
#    `users` list; see examples/values-kind.yaml).
helm dependency update
helm upgrade --install ecflow . -n ecflow --create-namespace -f examples/values-kind.yaml -f users.yaml
kubectl -n ecflow rollout status deploy/ecflow-server --timeout=180s
```

The checks, from the host. The reverse proxy answers on `https://ecflow.localtest.me`
(the controller's self-signed certificate; the client does not verify it), SFTP on
`localhost:2222`:

```bash
curl -sk https://ecflow.localtest.me/                       # the welcome page
curl -sk -o /dev/null -w '%{http_code}\n' https://ecflow.localtest.me/v1/ecflow   # 401: no credentials

cat > ecflowapirc <<'EOT'
{ "version": 1, "tokens": [ { "type": "basic", "server": "https://ecflow.localtest.me:443",
    "api": { "username": "admin", "password": "somesecret#admin" } } ] }
EOT
ECF_AUTHTOKENS=$PWD/ecflowapirc ecflow_client --https --host ecflow.localtest.me --port 443 --ping

sftp -P 2222 -o IdentitiesOnly=yes -i <key> <user>@localhost   # lands in /workspace
```

To remove everything: `kind delete cluster --name ecflow`.
