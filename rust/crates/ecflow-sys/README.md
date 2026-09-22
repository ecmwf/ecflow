<!--
SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
SPDX-License-Identifier: Apache-2.0
-->

# ecflow-sys

Low-level Rust bindings to the client side of ECMWF's
[ecFlow](https://github.com/ecmwf/ecflow) workflow manager.

This crate provides raw FFI bindings using [cxx](https://cxx.rs/) around the
C++ `ClientInvoker` class. For a safe API, use the higher-level
[`ecflow`](https://crates.io/crates/ecflow) crate.

## How it builds

The build script configures and builds the ecFlow C++ sources with CMake, with
ecbuild on `CMAKE_PREFIX_PATH`, compiles the bridge against the public include
directories and definitions of the `ecflow_all` target, and links that static
archive with its public libraries.

When the crate is built from the ecFlow repository, the in-tree sources are
used; otherwise the release matching the crate version is cloned.

## Cargo build features

- `ssl` (default) - Build with OpenSSL support (`ENABLE_SSL`).

## Environment variables

- `ECBUILD_DIR` - Path to an ecbuild checkout, instead of cloning it.
- `BOOST_ROOT`, `OPENSSL_ROOT_DIR`, `CMAKE_PREFIX_PATH` - Forwarded to CMake.
  On macOS the Homebrew prefixes are used when these are unset.
- `DOCS_RS` - When set, the build script becomes a no-op (for docs.rs).

## Copyright and License

Copyright 2009- European Centre for Medium-Range Weather Forecasts (ECMWF).

This software is licensed under the terms of the [Apache License, Version 2.0](LICENSE) which can also be obtained at http://www.apache.org/licenses/LICENSE-2.0.

In applying this licence, ECMWF does not waive the privileges and immunities granted to it by virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
