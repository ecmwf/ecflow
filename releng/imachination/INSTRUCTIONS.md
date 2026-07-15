# Running the `imachination` environment

`imachination` is a Docker Compose stack that reproduces a ecFlow deployment, where an ecFlow server sits behind a
reverse proxy that gates access through an authentication service.

It is intended for exercising the authentication path (Basic and Bearer tokens) end to end.

The stack has three services, defined in `compose.yaml`:

- `revproxy`

    - an nginx reverse proxy, built locally from `revproxy/Dockerfile`.

      Allows HTTPS requests, terminating TLS with a self-signed certificate, and gates the `/v1/ecflow` location behind
      an `auth_request` call to `authotron`.

- `authotron`

    - the `auth-o-tron` authentication service (`eccr.ecmwf.int/auth-o-tron/auth-o-tron`), configured via
      `authotron/config.yaml` allows two providers:

        - the real `ecmwf-api-provider` (validates Bearer tokens against `https://api.ecmwf.int/v1`)
        - a `plain-provider` with a fixed list of test users/passwords, for local Basic authentication testing.

- `ecflow`

    - the ecFlow server image (`eccr.ecmwf.int/ecflow-dev-environments/ecflow-serveronly-dev:latest`).

## Prerequisites

- Docker, with Compose (`docker compose`)

- Login access to the `eccr.ecmwf.int` registry, to pull the `authotron` and `ecflow` images

  _n.b. This can be enabled with: `docker login eccr.ecmwf.int`_

Instead of Docker, can also use Podman, with the `podman-compose` wrapper.
The instructions below are written for Docker, but should work with Podman as well.

## Configuring the ecFlow workspace

The `ecflow` service mounts a workspace directory into the container, at `/workspace`, and uses it as `ECF_HOME`.
By default, this is `ecflow/workspace` in this directory, which already contains a base `server_environment.cfg`,
so no configuration is required to get started.

This directory is controlled by the `WORKSPACE_DIR` environment variable. Setting it to an absolute path overrides
the default, using that path for the host-side mount, the container-side mount, the working directory, and `ECF_HOME`
alike, so that the server and any job scripts it generates all agree on the same path, for example:

```bash
export WORKSPACE_DIR="/var/ecflow/workspace"
```

## Starting the stack

Run, from this directory:

```bash
docker compose up --build
```

The `--build` option is required because `revproxy` has no pre-built image and must be built locally; `authotron` and
`ecflow` are pulled from the registry.

Once started, by default the following ports are published on the host:

| Port | Service     | Purpose                                                |
|------|-------------|--------------------------------------------------------|
| 80   | `revproxy`  | HTTP, redirects to HTTPS                               |
| 443  | `revproxy`  | HTTPS (self-signed certificate)                        |
| 8080 | `authotron` | Direct access to the authentication service            |
| 8888 | `ecflow`    | ecFlow server (native protocol, started with `--http`) |
| 8889 | `ecflow`    | `ecflow_http` REST API server                          |

### Starting the stack manually, without Compose

On systems where only plain `podman` is available, without the `podman-compose` wrapper, the same setup can be recreated
manually with the commands below. `docker` can be replaced with `podman` throughout.

The commands mirror the defaults from `compose.yaml`, including the `WORKSPACE_DIR` fallback described above; exporting
`WORKSPACE_DIR` before running them overrides that default the same way it does for `docker compose`.

Create the network shared by the three containers:

```bash
docker network create --subnet 172.30.0.0/16 inner
```

Build and run `revproxy`:

```bash
docker build -t imachination-revproxy ./revproxy

docker run -d \
    --name revproxy \
    --hostname revproxy \
    --network inner --ip 172.30.0.2 \
    -p 80:80 -p 443:443 \
    -v "$(pwd)/revproxy/server:/usr/share/nginx/html/server" \
    -v "$(pwd)/revproxy/cfgs/nginx/default.conf:/etc/nginx/conf.d/default.conf" \
    imachination-revproxy
```

Run `authotron`:

```bash
docker run -d \
    --name authotron \
    --hostname authotron \
    --network inner --ip 172.30.0.3 \
    -p 8080:8080 \
    -v "$(pwd)/authotron/config.yaml:/app/config.yaml" \
    eccr.ecmwf.int/auth-o-tron/auth-o-tron:0.2.8
```

Run `ecflow`:

```bash
docker run -d \
    --name ecflow \
    --hostname ecflow-server \
    --platform linux/amd64 \
    --network inner --ip 172.30.0.4 \
    -p 8888:8888 -p 8889:8889 \
    -v "${WORKSPACE_DIR:-$(pwd)/ecflow/workspace}:${WORKSPACE_DIR:-/workspace}" \
    -w "${WORKSPACE_DIR:-/workspace}" \
    -e "ECFLOW_WORKSPACE_DIR=${WORKSPACE_DIR:-/workspace}" \
    eccr.ecmwf.int/ecflow-dev-environments/ecflow-serveronly-dev:latest
```

To stop and remove the stack:

```bash
docker rm -f revproxy authotron ecflow
docker network rm inner
```

## Testing authentication through the reverse proxy

Requests to `https://<hostname>/v1/ecflow` are proxied to the ecFlow server only if the `auth_request` to `authotron`
succeeds. The option `-k` is required on `curl` because the certificate is self-signed.

Using Basic authentication, with one of the test users defined in
`authotron/config.yaml`:

```bash
BASIC_TOKEN=$(echo -n 'user:secret' | base64)
curl -k -X GET -H "Authorization: Basic ${BASIC_TOKEN}" https://localhost/v1/ecflow
```

Using Bearer authentication, with an ECMWF API key validated against the
`ecmwf-api-provider`:

```bash
BEARER_TOKEN=$(jq -r '.key' ~/.ecmwfapirc)
curl -k -X GET -H "Authorization: Bearer ${BEARER_TOKEN}" https://localhost/v1/ecflow
```
