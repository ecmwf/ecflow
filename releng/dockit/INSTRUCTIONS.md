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

The action `.github/workflows/dockit.yml` automates both image build processes described below, end-to-end,
on `workflow_dispatch`, as two jobs, each running once per image variant via a build matrix (preset is the
matrix dimension: each leg pairs a preset with the image name and Dockerfile directory it belongs to):

1. The `package` job builds the ecFlow Debian package inside `marcosbento/lumen:debian-13.5`, following the
   same checkout/configure/build/package steps as `ecflow-server.build.package.sh`, and uploads the resulting
   `.deb` as an `ecflow-debian-package-<image>` artefact.

2. The `dockerize` job, using the same matrix, downloads the matching artefact into the matching
   `ecflow-server/` or `ecflow-all/` directory and builds the Docker image from that directory's
   `Dockerfile`,  then pushes it to `eccr.ecmwf.int/ecflow-dev-environments/<image>`.

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
    --build-arg ECFLOW_PACKAGE=ecflow-<version>_<sha>-Linux_x86_64.deb \
    -t ecflow-serveronly-dev:latest \
    ecflow-server/
```

Note: provide the `.deb` filename explicitly to match the file produced in Step 1. To avoid defining version and
package,
rename the package to the default name `ecflow-latest-Linux.deb`.

The ecflow ports are configurable via the `ECFLOW_SERVER_PORT` (default `8888`) and `ECFLOW_REST_PORT` (default `8889`)
environment variables.

Run the image with, for example:

```bash
docker run --rm -p 8888:8888 -p 8889:8889 ecflow-serveronly-dev:latest
```

### Building the `ecflow-all` image

This section describes the process to build a Docker image capable of running `ecflow_ui`, connected to a remote ecFlow
server (for example on ECMWF's HPC) through a `proxychains4` + SSH SOCKS tunnel opened on the Docker host. Although it
has `ecflow-server`, this image is designed to run `ecflow_ui` only, and does not start a server itself.

#### Step 1 (repeated): Build the ecFlow Debian package

Follow Step 1 above to produce a `.deb`. In this case, because the package must include the ecflow UI, use preset
`linux.gcc.all.relwithdebinfo` for the configuration, for example:

```bash
./ecflow-server.build.package.sh --preset linux.gcc.all.relwithdebinfo --output_dir ecflow-all
```

Ensure this the `.deb` is made available in `ecflow-all/` to support building the Docker image in Step 3.
The `--output_dir` option above copies the package into that directory automatically.

#### Step 3: Build the `ecflow-all` container image

```bash
docker build \
    --build-arg ECFLOW_PACKAGE=ecflow-<version>_<sha>-Linux_x86_64.deb \
    -t ecflow-all-dev:latest \
    ecflow-all/
```

Note: provide the `.deb` filename explicitly to match the file produced in Step 1, or rename the
package to the default name `ecflow-latest-Linux.deb` to avoid defining version and package.

#### Running `ecflow_ui`

The `ecflow_ui` can easily be launched using the `docker_ecflow_ui.sh` script.

This utility script handles all the nitty-gritty details of using Docker to run `ecflow_ui`:

 - it forwards X11 to the host,
 - mounts and passes `-confd` for the ecflow_ui configuration directory
 - applies `xhost +`.

It is self-contained, so it can be run either as a local checkout or fetched and piped straight into `bash`:

```bash
./docker_ecflow_ui.sh
# or
curl -fsSL <url>/docker_ecflow_ui.sh | bash -s -- --proxychains=on
```

Options (`./docker_ecflow_ui.sh --help` for the full list):

| Option                  | Default                                                                     | Purpose                                                                                  |
|-------------------------|-----------------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| `-c`, `--cfg DIR`       | `~/.ecflow_ui_v5`                                                           | ecflow_ui configuration directory (bind-mounted, passed via `-confd`)                    |
| `-i`, `--image IMAGE`   | `ecflow-all-dev:latest`                                                     | Docker image to run                                                                      |
| `--proxychains=on\|off` | `off`                                                                       | Route connections through proxychains4 + the SSH SOCKS tunnel                            |
| `--authtokens FILE`     | unset (falls back to `ECF_AUTHTOKENS` from the calling environment, if set) | Auth tokens file, bind-mounted read-only and exposed to `ecflow_ui` via `ECF_AUTHTOKENS` |

By default (`--proxychains=off`) the script bypasses the image's own `ENTRYPOINT`(`launch-ecflow_ui.sh`) and runs
`ecflow_ui` directly — appropriate when the target ecFlow server is already reachable without a tunnel.

With `--proxychains=on`, the image's `ENTRYPOINT` resolves the SOCKS proxy host (`host.docker.internal` by default) to
a literal IP address at container start, generates `/etc/proxychains4.conf` from it, and execs `ecflow_ui` under
`proxychains4`. Two things are required on the host side in that case:

1. An SSH SOCKS tunnel to the HPC environment, bound to more than loopback so the container can reach it
   through `host.docker.internal`:

   ```bash
   ssh -g -D 9050 user@hpc-gateway
   ```

2. On macOS, an X server (e.g. XQuartz) running (`docker_ecflow_ui.sh` applies `xhost +` itself).
