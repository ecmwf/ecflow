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

The action `.github/workflows/dockit.yml` automates the `ecflow-server` image build process described below,
end-to-end, on `workflow_dispatch`:

1. The `package` job builds the ecFlow Debian package inside `marcosbento/lumen:debian-13.5`, following the same
   checkout/configure/build/package steps as `ecflow-server.build.package.sh`, and uploads the resulting `.deb`
   as a build artefact.

2. The `dockerize` job downloads that artifact into `ecflow-server/` and builds the Docker image from
   `ecflow-server/Dockerfile`, passing the same `ECFLOW_VERSION` and `ECFLOW_PACKAGE` build arguments described in the
   `ecflow-server` section below, then pushes it to `eccr.ecmwf.int/ecflow-dev-environments/ecflow-serveronly-dev`.

## Building the images

### Building the `ecflow-server` image

This section describes the process to build a Docker image running an ecFlow server.

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

The resulting `.deb` is copied into the output directory, which defaults to `ecflow-server/` (`${PWD}/ecflow-server`).
This is the same directory used as the Docker build context in Step 2, so no manual copy is needed with default
settings. Creating the package with a different `--output_dir` means the package must be moved into `ecflow-server/`
manually before Step 2.

The script accepts several options, for example to reuse an existing checkout (`--skip-checkout`), point at a different
branch or repository, or change the output directory. Run `./ecflow-server.build.package.sh --help` for the full list.

#### Step 2: Build the ecFlow server container image

The `ecflow-server/Dockerfile` installs the `.deb` file that must be present in its build context at build time.
This is typically the package generated in Step 1.

Build the image, passing the package version and filename as build arguments:

```bash
docker build \
    --build-arg ECFLOW_VERSION=<version>_<sha> \
    --build-arg ECFLOW_PACKAGE=ecflow-<version>_<sha>-Linux_x86_64.deb \
    -t ecflow-serveronly-dev:latest \
    ecflow-server/
```

Note: provide `.deb` filename explicitly to match the file produced in Step 1. To avoid defining version and package,
rename the package to the default name `ecflow-latest-Linux.deb`.

The ecflow ports are configurable via the `ECFLOW_SERVER_PORT` (default `8888`) and `ECFLOW_REST_PORT` (default `8889`)
environment variables.

Run the image with, for example:

```bash
docker run --rm -p 8888:8888 -p 8889:8889 ecflow-serveronly-dev:latest
```
