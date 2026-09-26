<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# Running `imachination` on a local Kubernetes cluster

This directory deploys the `imachination` stack described in [`../INSTRUCTIONS.md`](../INSTRUCTIONS.md) on a
local Kubernetes cluster created with [kind](https://kind.sigs.k8s.io/), instead of Docker Compose. The three
services are the same: an ecFlow server behind an nginx reverse proxy, which authenticates every request with
`auth-o-tron`. The deployment is intended for development and testing on a single machine.

Everything is driven by one script, `imachination.sh`. It can be run from any directory; the examples below are
run from the `imachination` directory, the parent of this one.

## Prerequisites

- Docker, running, with enough memory for a Kubernetes node (4 GB or more is recommended).
- [kind](https://kind.sigs.k8s.io/) and `kubectl`.
- Login access to the `eccr.ecmwf.int` registry, from which the ecFlow and `auth-o-tron` images are pulled:

  ```bash
  docker login eccr.ecmwf.int
  ```

- To use the deployment, an ecFlow client (`ecflow_client`, `ecflow_ui`) built with SSL support, recent enough
  to read credentials from an `ecflowapirc` file (see "Using ecFlow through the reverse proxy").

## Quick start

```bash
k8s/imachination.sh up        # create the cluster, load the images, and deploy the stack
k8s/imachination.sh verify    # check the authenticated path with ecflow_client
k8s/imachination.sh status    # show the cluster, the loaded images and the objects of the stack
```

`up` takes about a minute when the images are already cached, and can be run again at any time: every
step it performs is idempotent. `verify` requires `ecflow_client` on `PATH`, and checks that valid credentials
reach the server, that missing and wrong credentials are refused with 401, and that the refused requests never
reach the server.

## Commands

| Command | Effect |
|---------|--------|
| `up` | Create the cluster, load the images and apply the stack, in one go |
| `cluster` | Render the cluster definition (into `k8s/.generated/`) and create the cluster, if absent |
| `images` | Build the reverse proxy image, pull the other two when not cached, and load all three into the cluster |
| `apply` | Declare the stack in the cluster and wait for every workload to become available |
| `verify` | Exercise the authenticated path with `ecflow_client`, from the host |
| `status` | Report the state of the cluster, its images and the stack |
| `logs [workload]` | Follow the output of every workload, or of the one named |
| `restart [workload]` | Replace every workload, or the one named, so that it re-reads its configuration |
| `reload [workload]` | Rebuild and pull the images again, even when cached, load them, and restart onto them |
| `down` | Delete the stack, keeping the cluster and its images |
| `destroy` | Delete the cluster and the rendered definition |

The workloads are `ecflow-server`, `authotron` and `revproxy`. The script reads the following environment
variables:

| Variable | Purpose |
|----------|---------|
| `WORKSPACE_DIR` | Workspace mounted into the ecFlow server (default: `../ecflow/workspace`) |
| `ECFLOW_SOURCE` | Source image of the ecFlow server (default: `ecflow-server-dev:latest`, published from `develop`) |
| `AUTHOTRON_SOURCE` | Source image of `auth-o-tron` |
| `ROLLOUT_TIMEOUT` | How long to wait for a workload to become available (default: `300s`) |
| `VERIFY_USER`, `VERIFY_PASSWORD` | Credentials used by `verify` (default: the test user `admin`) |

## What is deployed

All objects live in the namespace `imachination`. Only the reverse proxy is reachable from the host:

| Host port | Service | Purpose |
|-----------|---------|---------|
| 443 | `revproxy` | HTTPS, with a self-signed certificate; `/v1/ecflow` is gated by `auth-o-tron` |

Unlike the Compose stack, nothing listens on the host on ports 80, 8080 or 8888: the authentication service and
the ecFlow server are reachable only from within the cluster, so the authenticated path is the only way in.

## Users and credentials

The users are those of the `plain-provider` in [`../authotron/config.yaml`](../authotron/config.yaml) (test
users only, such as `admin` and `mamb`), plus any ECMWF user holding an API key, validated by the
`ecmwf-api-provider` against `https://api.ecmwf.int/v1`. Validating an API key requires the cluster to reach
that address; without outbound access, only the test users can log in.

After editing `config.yaml`, run `k8s/imachination.sh apply`: the configuration is carried as a generated Secret,
whose name changes with its content, so the `auth-o-tron` workload is replaced automatically.

## Using ecFlow through the reverse proxy

Every client, interactive or not, reaches the server as `https://localhost:443` with the `--https` option, and
provides its credentials through an `ecflowapirc` file. The client looks for the file named by
`ECF_AUTHTOKENS`, then for `~/.ecflowapirc`. Each entry applies to the server whose URL,
`https://<host>:<port>`, matches its `server` field, a regular expression:

```json
{
  "version": 1,
  "tokens": [
    { "type": "basic", "server": "https://localhost:443",
      "api": { "username": "admin", "password": "somesecret#admin" } },
    { "type": "bearer", "server": "https://localhost:443",
      "api": { "url": "https://api.ecmwf.int/v1", "key": "<API key>", "email": "<email>" } }
  ]
}
```

Only the first matching entry is used, so a file holds either the Basic or the Bearer entry for a given server.
The client does not verify the certificate of the server, so the self-signed certificate of the reverse proxy
needs no arrangement.

```bash
export ECF_HOST=localhost ECF_PORT=443 ECF_AUTHTOKENS=$HOME/.ecflowapirc
ecflow_client --https --ping
ecflow_client --https --stats
```

In `ecflow_ui`, add the server `localhost`, port `443`, using HTTPS; it reads the same file.

The ecFlow server starts **halted**, including after a `restart` or a `reload`: no job is submitted until an
`ecflow_client --https --restart` (or the corresponding action in `ecflow_ui`) is issued. The suites are
recovered from the checkpoint file, which is kept in the workspace.

## From a suite on disk to a suite running on the server

The workspace directory (`../ecflow/workspace` by default) is mounted into the ecFlow server as `/workspace`,
which is also its `ECF_HOME`. The server reads its configuration from `/workspace/server_environment.cfg`, and
`ECF_FILES` is set to `/workspace/files`. Files written in the workspace on the host are therefore seen by the
server immediately, and job outputs written by the server appear on the host.

To run a suite:

1. Copy the task scripts and include files into the workspace, under the paths that the suite definition
   declares (`ECF_FILES`, `ECF_INCLUDE`), and create the directory tree of `ECF_OUT`, which the server does not
   create. When the workspace is not shared with the host, `kubectl cp` provides the same:

   ```bash
   POD=$(kubectl -n imachination get pod -l app=ecflow-server -o name | cut -d/ -f2)
   kubectl -n imachination cp <local directory> "${POD}:/workspace/<directory>"
   ```

2. Load the definition from the host; the paths it contains are those seen by the server (`/workspace/...`):

   ```bash
   ecflow_client --https --load <suite>.def
   ecflow_client --https --begin <suite>
   ```

The jobs run inside the ecFlow server container, where `ecflow_client` is `/usr/local/bin/ecflow_client` and
troika is installed in the virtual environment `/opt/local/python`. The child commands of the jobs
(`--init`, `--complete`, and so on) must reach the server through the reverse proxy as well, with `--https`
and an `ecflowapirc` file readable in the container. Inside the cluster, the reverse proxy is
`revproxy.imachination.svc.cluster.local:443`, and the `ecflowapirc` file must hold an entry for that address.
As `ECF_HOST` and `ECF_PORT` are generated by the server and cannot be overridden by a suite, hold the address
of the proxy in variables of the suite's own, and export them as `ECF_HOST` and `ECF_PORT` in the job header,
for example:

```text
edit ECFLOW_PROXY_HOST 'revproxy.imachination.svc.cluster.local'
edit ECFLOW_PROXY_PORT '443'
```

```bash
export ECF_HOST=%ECFLOW_PROXY_HOST%
export ECF_PORT=%ECFLOW_PROXY_PORT%
export ECF_AUTHTOKENS=/workspace/secrets/%OWNER%/ecflowapirc
```

## Changing the configuration

| After editing | Run |
|---------------|-----|
| `ecflow/workspace/server_environment.cfg` | `k8s/imachination.sh restart ecflow-server`, as the server reads the file only at start-up |
| `authotron/config.yaml`, `k8s/revproxy/nginx-default.conf`, any manifest | `k8s/imachination.sh apply` |
| A new image published upstream, or `revproxy/` | `k8s/imachination.sh reload`, then `--restart` the ecFlow server |

## Troubleshooting

- `k8s/imachination.sh logs` follows the output of every workload; `apply`, `restart` and `reload` print the
  events and the last output of any workload that fails to become available.
- The ecFlow server Deployment lowers the open files limit of its container to 1048576 before starting the
  server. The container runtime of the kind node grants a limit of about a billion, and the server closes every
  descriptor up to that limit in each process it forks for a job, which would otherwise keep every job from
  starting.
- A job that stays `submitted` with no output usually points at a missing `ECF_OUT` directory, or at a child
  command that cannot reach the server; the job output shows the error of `ecflow_client`.

## Known limitations

- Within the cluster, the ecFlow server Service (`ecflow-server`, port 8888) accepts plain HTTP requests
  without authentication from any Pod, bypassing the reverse proxy. The host is not exposed to it.
- The deployment is not meant for production: the credentials in `config.yaml` are test values, and the
  certificate of the reverse proxy is self-signed.

## Removing the deployment

```bash
k8s/imachination.sh down       # delete the stack, keeping the cluster and its images
k8s/imachination.sh destroy    # delete the cluster
```

The workspace, with the suites, their outputs and the checkpoint of the server, is left on the host.
