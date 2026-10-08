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

## The test deployment on webapps-test

The profile `examples/values-webapps-test.yaml` deploys the chart in the namespace
`ecflow-multitenant-test` of the ECMWF cluster webapps-test, published on
`ecflow-mt-test.ecmwf.int` through the NGINX Inc controller, with a certificate from
cert-manager and a name visible on the ECMWF LAN only. The cluster is reached through
Teleport (`tsh`); the images are pulled from `eccr.ecmwf.int` by the cluster itself.
Run the steps from this directory.

```bash
# 0. Session and context (the proxy keeps running in a terminal of its own)
tsh login --proxy=jump-test-18.ecmwf.int --user=$(whoami)
tsh kube login webapps-test
tsh proxy kube webapps-test          # prints the KUBECONFIG to export
export KUBECONFIG=<path printed by tsh proxy kube>
N=ecflow-multitenant-test
kubectl get all,secret,configmap,pvc,ingress,networkpolicy -n $N

# 1. The controller of the cluster, for the NetworkPolicy of the reverse proxy: its
#    namespace and Pod labels must match networkPolicy.ingressController.nginx-inc
#    (override them with --set below when they differ).
kubectl get ingressclass
kubectl get pods -A --show-labels | grep -i 'nginx-ingress\|ingress-nginx'

# 2. The users file (not committed): the SSH public key of each SFTP account
KEY=~/.ssh/id_ed25519                # the key pair of the deployer
cat > users.yaml <<EOF
users:
  - name: $(whoami)
    sshAuthorizedKeys:
      - $(cat $KEY.pub)
EOF

# 3. Render, dry run, deploy
helm dependency update
helm template ecflow . -n $N -f examples/values-webapps-test.yaml -f users.yaml | less
helm install ecflow . -n $N -f examples/values-webapps-test.yaml -f users.yaml --dry-run
helm upgrade --install ecflow . -n $N -f examples/values-webapps-test.yaml -f users.yaml
sleep 30; kubectl -n $N get pods      # not `rollout status`: see the note on watches below
kubectl -n $N get pods,svc,pvc,ingress,networkpolicy
kubectl -n $N get certificate                     # Ready once cert-manager issued it
kubectl -n $N describe ingress ecflow-revproxy    # events of the controller and of the DNS operator
```

The checks, from the ECMWF LAN. The name resolves there only; the certificate is a
real one, so no `-k`:

```bash
dig +short ecflow-mt-test.ecmwf.int
curl -s -o /dev/null -w '%{http_code}\n' https://ecflow-mt-test.ecmwf.int/               # 200
curl -s -o /dev/null -w '%{http_code}\n' https://ecflow-mt-test.ecmwf.int/v1/ecflow      # 401

cat > ecflowapirc <<'EOT'
{ "version": 1, "tokens": [ { "type": "basic", "server": "https://ecflow-mt-test.ecmwf.int:443",
    "api": { "username": "admin", "password": "somesecret#admin" } } ] }
EOT
ECF_AUTHTOKENS=$PWD/ecflowapirc ecflow_client --https --host ecflow-mt-test.ecmwf.int --port 443 --ping

# The four checks of `helm test`, run as a plain Pod (see the note on watches below)
helm template ecflow . -n $N -f examples/values-webapps-test.yaml -f users.yaml \
    --show-only templates/tests/auth-test.yaml | kubectl -n $N apply -f -
sleep 20; kubectl -n $N get pod ecflow-test-auth; kubectl -n $N logs ecflow-test-auth
kubectl -n $N delete pod ecflow-test-auth

kubectl -n $N port-forward svc/ecflow-sftp 18022:22 &   # SFTP, until the TransportServer is enabled
sftp -P 18022 -o IdentitiesOnly=yes -i $KEY $(whoami)@localhost
kill %1
```

While the DNS record of the name has not reached the local resolver, the `curl` checks
work with `--resolve ecflow-mt-test.ecmwf.int:443:<address of the Ingress>`, which keeps
the server name and the `Host` header.

Through `tsh proxy kube`, the watch stream of the API delivers no events, so every
command that waits on a watch fails after its timeout although the cluster did the work:
`kubectl rollout status`, `kubectl wait`, `kubectl run -i --rm`, and `helm test`, which
then deletes its Pod. Use a pause followed by `kubectl get` instead, and run the test
Pod by hand as above.

To remove the deployment: `helm uninstall ecflow -n $N`; the PVCs of the workspace and
the state are kept, and deleted with `kubectl -n $N delete pvc ecflow-workspace ecflow-state`.
