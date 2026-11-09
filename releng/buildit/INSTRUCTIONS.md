<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# Building ecFlow from source

This folder contains scripts that build and install ecFlow from source. Each build script is self-contained, and
performs the following tasks:

  - clones `ecflow` (and build-time dependency `ecbuild`)
  - configures the project with CMake
  - builds ecFlow
  - installs ecFlow

The folder also contains scripts that provision the build-time dependencies not commonly available on the build
hosts (see `provision_aviso.sh` below).

## Build ecFlow on the HPC (`build.hpc.sh`)

This script builds and installs ecFlow on ECMWF HPC systems, using the Lmod `module` environment to load one of several
pre-defined compiler/Python/Boost/Qt toolchain combinations.

### Prerequisites

- An Lmod-based environment (the `module` command available), such as an ECMWF HPC login or compute node.
- Network access to clone `ecflow` and `ecbuild` from GitHub, unless
  `--skip-checkout` is used with an existing checkout.

### Usage

Run directly, from a checkout of this repository:

```bash
./build.hpc.sh
```

Or, without a prior checkout, piped from its raw URL:

```bash
curl -fsSL https://raw.githubusercontent.com/ecmwf/ecflow/develop/releng/buildit/build.hpc.sh \
    | bash -s -- --build_dir /path/to/sandbox
```

When using the default options, this script clones `ecflow` and `ecbuild` (`develop` branch) into `${PWD}/sandbox`,
loads the default toolchain (`gcc8qt5`) configuration, and configures, builds, and installs ecFlow considering
build type `RelWithDebInfo`.

The install prefix defaults to `<build_dir>/.install/<tag>`, where `<tag>` encodes the build type, configuration, and
architecture, for example `linux.relwithdebinfo.gcc8qt5_x86_64`.

The toolchain configuration, build type, branches/repositories, install location, parallelism, and which optional
components are built (UI, Python bindings, tests) can all be overridden. Run `./build.hpc.sh --help` for the full list
of options and available toolchain configurations.

## Provision the Aviso client library (`provision_aviso.sh`)

This script provisions the Aviso client library (`libaviso_ffi`), required to build ecFlow with `ENABLE_AVISO=ON`. It
downloads the prebuilt aviso-ffi release archive of [aviso-client](https://github.com/ecmwf/aviso-client) for the
platform it runs on, verifies its SHA-256 checksum, and unpacks it into a destination directory.

The CI builds on the Linux runners and on the HPC call it as a preliminary step, before configuring ecFlow (see
`.ci/runner/build.sh.j2` and `.ci/hpc/build.sh.j2`).

### Prerequisites

- Linux (x86_64 or aarch64, glibc 2.28 or later) or macOS (x86_64 or arm64).
- Network access to download the release archive from GitHub.
- `curl` or `wget`, and `sha256sum` or `shasum`.

### Usage

The directory of the unpacked release (i.e. the directory containing `include/aviso_ffi/` and `lib/`) is the only
output written to stdout, and is given to CMake as `AVISO_FFI_ROOT`:

```bash
AVISO_FFI_ROOT="$(./provision_aviso.sh --dest /path/to/deps)"
cmake ... -DENABLE_AVISO=ON -DAVISO_FFI_ROOT="${AVISO_FFI_ROOT}"
```

Notes:

- The destination directory defaults to `${TMPDIR:-/tmp}/aviso-ffi`.
- A release already unpacked in the destination directory is reused, without being downloaded again.
- The static library of the release is discarded, as ecFlow only links the shared library.
- The installed `ecflow_server` finds the shared library through its run path, set to the directory where the library
  was found at build time; the destination directory must therefore be kept for as long as the installation is used.

The release defaults to the version pinned in the script, and is selected with `--version`. Only releases with a known
checksum are accepted, unless the expected checksum is given with `--sha256`. Run `./provision_aviso.sh --help` for the
full list of options.
