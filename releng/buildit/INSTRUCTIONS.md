# Building ecFlow from source

This folder contains scripts that build and install ecFlow from source. Each script is self-contained, and performs the
following tasks:

  - clones `ecflow` (and build-time dependency `ecbuild`)
  - configures the project with CMake
  - builds ecFlow
  - installs ecFlow

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
