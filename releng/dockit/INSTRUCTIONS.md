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

1. The `package` job builds the ecFlow Debian package inside `marcosbento/lumen:debian-13.5`, following the
   same checkout/configure/build/package steps as `ecflow-server.build.package.sh`, once per architecture
   (`amd64` and `arm64`), each natively on a GitHub-hosted runner of that architecture. Each leg names its
   package `ecflow-<arch>.deb` and uploads it as an `ecflow-debian-package-<image>-<arch>` artefact.

2. The `dockerize` job, using the same matrix, downloads the packages of all architectures into the matching
   Dockerfile directory (`ecflow-server/`) and builds the Docker image from that directory's
   `Dockerfile` for `linux/amd64` and `linux/arm64` at once (the `arm64` image under QEMU emulation, which only
   installs the package), then pushes it to `eccr.ecmwf.int/ecflow-dev-environments/<image>` as a single
   multi-platform image.

## Building the images

### Building the `ecflow-server` image

This section describes the process to build the `ecflow-server-dev` Docker image, running an ecFlow server.

As mentioned above, this is automated in the `dockit` workflow, but can also be done manually as follows.

#### Step 1: Build the ecFlow Debian package

Run:

```bash
./ecflow-server.build.package.sh
```

This launches the `marcosbento/lumen:debian-13.5` Docker image and, inside it:

1. Checks out `ecbuild` and `ecflow` (`develop` branch, by default).

2. Configures ecflow with the default preset and `-DCUSTOM_DEBIAN_PACKAGE_VERSION=<project version>_<git sha>`.

3. Builds ecflow (`cmake --build --target all`).

4. Packages ecflow as a Debian package (`cmake --build --target package`).

With the default preset, `linux.gcc.server.release`, the package declares the runtime libraries its binaries link
against as dependencies, as derived by `dpkg-shlibdeps`. This requires the `file` utility in the build environment
image.

The resulting package is named after its architecture, `ecflow-<arch>.deb` (e.g. `ecflow-amd64.deb`, or
`ecflow-arm64.deb` on Apple Silicon), and copied into the output directory, which defaults to `ecflow-server/`
(`${PWD}/ecflow-server`).
This is the same directory used as the Docker build context in Step 2, so no manual copy is needed with default
settings. Creating the package with a different `--output_dir` means the package must be moved into `ecflow-server/`
manually before Step 2.

The script accepts several options, for example to reuse an existing checkout (`--skip-checkout`), point at a different
branch or repository, or change the output directory. Run `./ecflow-server.build.package.sh --help` for the full list.

#### Step 2: Build the ecFlow server container image

The `ecflow-server/Dockerfile` installs the package for the target platform, `ecflow-<arch>.deb`, which must be
present in its build context at build time. This is typically the package generated in Step 1.

Build the image for the platform of the Docker host with:

```bash
docker build \
    -t ecflow-server-dev:latest \
    ecflow-server/
```

The package is selected by the target architecture (the `TARGETARCH` build argument, set by BuildKit), so no package
name needs to be given. A multi-platform image, as published by the workflow, requires the packages of all the
target architectures in the build context:

```bash
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    -t ecflow-server-dev:latest \
    ecflow-server/
```

The image installs the package with `apt-get`, together with the runtime libraries the package depends on.

The ecflow server port is configurable via the `ECFLOW_SERVER_PORT` environment variable (default `8888`).

Run the image with, for example:

```bash
docker run --rm -p 8888:8888 ecflow-server-dev:latest
```

Any arguments given to the container are passed on to `ecflow_server` (for example, `-d` for debug output). A health
check pings the server with `ecflow_client --ping`. On `docker stop`, the server is asked to terminate cleanly with
`ecflow_client --terminate`.

The server runs as the unprivileged user `ecflow` (UID and GID 1000, by default), and a workspace bind-mounted at
`/workspace` must be writable by that user. As ecFlow identifies users by their `/etc/passwd` entry, the container
cannot run under an arbitrary `docker run --user <uid>`; instead, build the image with a matching user, for example
by adding `--build-arg ECFLOW_UID=$(id -u) --build-arg ECFLOW_GID=$(id -g)` to the `docker build` command.
