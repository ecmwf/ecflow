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
