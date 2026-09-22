<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# ecFlow Rust bindings

Rust bindings for the client side of ECMWF's [ecFlow](https://github.com/ecmwf/ecflow)
workflow manager.

## Overview

ecFlow jobs and tools talk to an ecFlow server through the C++ `ClientInvoker`
class, which the `ecflow_client` command line tool and the Python module both
wrap. These crates make the same class usable from Rust: the C++ library is
built from source at build time and exposed through a safe API covering
connection configuration, server probes, the child (task) commands and
definitions as text. Every other command is reachable through
`Client::invoke`, which takes `ecflow_client` command line arguments.

## Installation

```toml
[dependencies]
ecflow = "5.19"
```

Building requires CMake, a C++17 compiler, Git and Boost 1.66 or newer with its
development headers. OpenSSL is needed for the default `ssl` feature. ecbuild
is cloned during the build unless `ECBUILD_DIR` points at a checkout.

`BOOST_ROOT` must point at the Boost install prefix, as for any ecFlow build:

```sh
BOOST_ROOT=$(brew --prefix boost) cargo build
```

## Crates

- **ecflow**: the safe API. `Client` for user and child commands, `Error` with
  the failure class ecFlow diagnosed.
- **ecflow-sys**: the low-level FFI layer using [CXX](https://cxx.rs/). Builds
  the ecFlow C++ library and exposes the raw bridge.

## Copyright and License

Copyright 2009- European Centre for Medium-Range Weather Forecasts (ECMWF).

This software is licensed under the terms of the [Apache License, Version 2.0](LICENSE) which can also be obtained at http://www.apache.org/licenses/LICENSE-2.0.

In applying this licence, ECMWF does not waive the privileges and immunities granted to it by virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
