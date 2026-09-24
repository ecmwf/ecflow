<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# Building ecFlow Docker images

This document describes the overall process to build ecFlow Docker images.

Building an image typically follows a two-step pattern:

- build the relevant ecFlow Debian package
- build a Docker image that installs that package.

All commands below are run from `ecflow/releng/dockit/`, unless otherwise
stated.

## Prerequisites

- Docker, installed and running.

## Relationship to the `dockit` GitHub Actions workflow

The action `.github/workflows/dockit.yml` automates the image build process described below, end-to-end,
on `workflow_dispatch`, as two jobs sharing a build matrix (each leg pairs a preset with the image name and
Dockerfile directory it belongs to):

1. The `package` job builds the ecFlow Debian package by running `build_ecflow_package_in_container.sh --source` on the
   checked-out commit, inside the build environment image, once per architecture (`amd64` and `arm64`), each
   natively on a GitHub-hosted runner of that architecture. This is the same script that
   `build_ecflow_package.sh` runs in its container (Step 1 below), so both build the package in exactly the
   same way. Each leg uploads the resulting `ecflow-<arch>.deb` as an `ecflow-debian-package-<image>-<arch>`
   artefact.

2. The `dockerize` job, using the same matrix, downloads the packages of all architectures into the matching
   Dockerfile directory (`ecflow-server/`) and runs `create_ecflow_docker_image.sh` (Step 2 below) to create the
   Docker image from that directory's `Dockerfile` for `linux/amd64` and `linux/arm64` at once (the `arm64` image
   under QEMU emulation, which only installs the package), and to push it to
   `eccr.ecmwf.int/ecflow-dev-environments/<image>` as a single multi-platform image. Before pushing, the image of
   each platform is smoke-tested: it must not exceed 400 MB (which catches, for example, a build carrying debug
   information) and must report a healthy server. The pushed image records the preset its package was built with
   in the `int.ecmwf.ecflow.preset` label.

The image is tagged after the branch the workflow runs from, using a slug of the branch name: the first component
of the name, if any, is dropped (e.g. `task/`), and the rest is lowercased, with anything other than letters, digits
and `.` replaced by `-` (e.g. `task/support_acl` becomes `support-acl`). Each run pushes:

- `<branch-slug>_<short-sha>_<timestamp>_<version>`, identifying the build (e.g.
  `support-acl_5bb07be94b71_20260924T123456_5.19.0`);
- `<branch-slug>`, pointing at the latest build of the branch;
- `latest`, only when running from `develop`, pointing at the latest build of `develop`.

## Building the images

### Building the `ecflow-server` image

This section describes the process to build the `ecflow-server-dev` Docker image, running an ecFlow server.

As mentioned above, this is automated in the `dockit` workflow, but can also be done manually as follows.

#### Step 1: Build the ecFlow Debian package

Run:

```bash
./build_ecflow_package.sh
```

This launches the build environment image (see `--docker-image`) and runs `build_ecflow_package_in_container.sh`
inside it, which holds the whole package build (and is also what the `dockit` workflow runs):

1. Checks out `ecbuild` (tag `3.16.0`, by default) and `ecflow` (`develop` branch, by default), unless the ecflow
   sources are given with `--source` (see below). The ecbuild tag, like the `troika` version installed by the image
   (`0.2.7`), is pinned so that a new release of either cannot change or break a build.

2. Configures ecflow with the default preset and `-DCUSTOM_DEBIAN_PACKAGE_VERSION=<project version>_<git sha>`.

3. Builds ecflow (`cmake --build --target all`).

4. Packages ecflow as a Debian package (`cmake --build --target package`).

The package declares the runtime libraries its binaries link against as dependencies, as derived by
`dpkg-shlibdeps`, together with the Python version its Python module is built for. This requires the `file` utility
in the build environment image.

The package version identifies the build: `<version>+git<commit time>.<commit>` (e.g.
`5.19.0+git20260924132851.20d90280fbce`, with the commit time in UTC), which Debian orders after the `<version>`
release, and among builds by commit time. A build of the commit tagged `<version>` keeps the plain `<version>`.

The resulting package is named after its architecture, `ecflow-<arch>.deb` (e.g. `ecflow-amd64.deb`, or
`ecflow-arm64.deb` on Apple Silicon), and copied into the output directory, which defaults to `ecflow-server/`
(`${PWD}/ecflow-server`).
This is the same directory used as the Docker build context in Step 2, so no manual copy is needed with default
settings. Creating the package with a different `--output_dir` means the package must be moved into `ecflow-server/`
manually before Step 2.

To build the ecflow sources of a local working tree (including uncommitted changes) instead of a fresh clone, pass
`--source`, for example from `ecflow/releng/dockit/`:

```bash
./build_ecflow_package.sh --source ../..
```

The sources are mounted read-only into the container, and the build tree is kept in the sandbox directory
(`--build_dir`), so the working tree is left untouched. When the git metadata of the sources is not usable inside the
container (e.g. in a git submodule), the package keeps the plain `<version>`; the package file name still carries the
commit, determined on the host.

The script accepts several options, for example to reuse an existing checkout (`--skip-checkout`), point at a different
branch or repository, or change the output directory. Run `./build_ecflow_package.sh --help` for the full list;
the options of the package build itself, and their defaults (e.g. the pinned ecbuild tag), are those of
`./build_ecflow_package_in_container.sh --help`.

#### Step 2: Build the ecFlow server container image

The `ecflow-server/Dockerfile` installs the package for the target platform, `ecflow-<arch>.deb`, which must be
present in its build context at build time. This is typically the package generated in Step 1.

Create the image for the platform of the Docker host, as `ecflow-server-dev:local`, with:

```bash
./create_ecflow_docker_image.sh
```

The script checks that the package of each target platform is present, and builds the image with
`docker buildx build`. The package is selected by the target architecture (the `TARGETARCH` build argument, set by
BuildKit), so no package name needs to be given. A multi-platform image, as published by the workflow, requires the
packages of all the target architectures in the build context:

```bash
./create_ecflow_docker_image.sh --platform linux/amd64,linux/arm64 --tag ecflow-server-dev:latest
```

With `--smoke-test`, the image of each platform is first built, checked against the size limit (`--max-size-mb`,
400 MB by default) and started, and must report a healthy server, as in the `dockit` workflow. With `--push`, the
image is pushed to its registry instead of being loaded into the local Docker. Run
`./create_ecflow_docker_image.sh --help` for all the options (tags, labels, build arguments, build context).

The image installs the package with `apt-get`, together with the runtime libraries the package depends on.
The package installs the `ecflow` Python module under `/usr/local/lib/python3.<minor>/dist-packages`, where Debian's
`python3` finds it; the virtual environment at `/opt/local/python` (activated for the server, with `troika`) includes
the system site packages, so the module is available there too.

The ecflow server port is configurable via the `ECFLOW_SERVER_PORT` environment variable (default `8888`).

Run the image with, for example:

```bash
docker run --rm -p 8888:8888 ecflow-server-dev:latest
```

Any arguments given to the container are passed on to `ecflow_server` (for example, `-d` for debug output). A health
check pings the server with `ecflow_client --ping`. On `docker stop`, the server is asked to terminate cleanly with
`ecflow_client --terminate`.

The server runs as the unprivileged user `ecflow`. The container starts as root, gives `ecflow` the UID and GID of
the owner of `/workspace` (typically a directory bind-mounted from the host), and then drops privileges to `ecflow`,
so that the server can write to the workspace whatever UID owns it on the host. A root-owned workspace, such as a host
directory that Docker had to create, is handed over to `ecflow` when empty. Without a mounted workspace, `ecflow` keeps
UID and GID 1000, which can be changed with `--build-arg ECFLOW_UID=... --build-arg ECFLOW_GID=...`.

As ecFlow identifies users by their `/etc/passwd` entry, the container must not be started with an arbitrary
`docker run --user <uid>`. To run commands as the server user in a running container, use
`docker exec -u ecflow <container> ...`.

#### Smoke-testing a published image

The following steps check an image published by the `dockit` workflow by hand, for example after a run from a branch.
They are run from an empty directory, and use the image of the branch `task/improve_dockit` (tag `improve-dockit`) as
an example; `--platform` selects the architecture to test (by default, that of the Docker host).

1. Pull the image, and check its architecture and labels:

   ```bash
   IMAGE=eccr.ecmwf.int/ecflow-dev-environments/ecflow-server-dev:improve-dockit
   docker pull --platform linux/arm64 "${IMAGE}"
   docker image inspect "${IMAGE}" --format '{{.Architecture}} {{json .Config.Labels}}'
   ```

2. Start a container, with the server port published on the host (here, as port `18888`) and a workspace
   bind-mounted from the host, and wait until the health check reports `healthy` (a few seconds):

   ```bash
   mkdir -p workspace
   docker run -d --name ecflow-smoke --platform linux/arm64 -p 18888:8888 -v "${PWD}/workspace:/workspace" "${IMAGE}"
   docker inspect -f '{{.State.Health.Status}}' ecflow-smoke
   ```

3. Ping the server from inside the container, as the server user `ecflow`, and load a test suite. As the server is
   started with `--http`, every client command must also use `--http`:

   ```bash
   docker exec -u ecflow ecflow-smoke ecflow_client --http --host localhost --port 8888 --ping
   docker exec -u ecflow ecflow-smoke bash -c 'printf "suite smoke\n  task t\nendsuite\n" > /tmp/smoke.def'
   docker exec -u ecflow ecflow-smoke ecflow_client --http --host localhost --port 8888 --load /tmp/smoke.def
   docker exec -u ecflow ecflow-smoke ecflow_client --http --host localhost --port 8888 --suites
   ```

   The ping reports `ping server(localhost:8888) succeeded`, and `--suites` lists `smoke`.

4. Optionally, ping the server from the host, through the published port, with an `ecflow_client` built or installed
   on the host:

   ```bash
   ecflow_client --http --host localhost --port 18888 --ping
   ecflow_client --http --host localhost --port 18888 --server_version
   ```

5. Stop the container, which asks the server to terminate cleanly, and check the result:

   ```bash
   docker stop ecflow-smoke
   docker inspect -f '{{.State.ExitCode}}' ecflow-smoke
   docker rm ecflow-smoke
   ls -l workspace
   ```

   The exit code is `0`, and the workspace holds the server log (`ecflow-server.8888.ecf.log`) and check-point
   (`ecflow-server.8888.ecf.check`, which contains the `smoke` suite), owned by the host user.

`create_ecflow_docker_image.sh --smoke-test` performs the automated part of these checks (image size, and a healthy
server) before an image is loaded or pushed.
