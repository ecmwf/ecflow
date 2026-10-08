.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _rust_api:

Rust API
********

..
   This reStructured Text file uses the following convention:
     # with overline, for parts
     * with overline, for chapters
     = for sections
     - for subsections
     ^ for subsubsections
     " for paragraphs

The ecFlow Rust API is provided by the ``ecflow`` crate. Its ``Client`` wraps the C++ ``ClientInvoker`` class, the
same class behind :ref:`ecflow_client <ecflow_cli>` and the ``Client`` of the :ref:`Python API <python_api>`, and has
one method per command.

Compilation
===========

The crates are in the ``rust`` directory of the ecFlow repository:

- **ecflow**, the safe API
- **ecflow-sys**, the bridge to the C++ library, on which ``ecflow`` depends

Add ``ecflow`` to the dependencies in ``Cargo.toml``:

.. code-block:: toml

  [dependencies]
  ecflow = "5.19"

Alternatively, take the crate from the ecFlow repository, at a branch, tag or revision:

.. code-block:: toml

  [dependencies]
  ecflow = { git = "https://github.com/ecmwf/ecflow", branch = "develop" }

The build script of ``ecflow-sys`` builds the ecFlow C++ client library with CMake and links it statically, so the
build needs CMake, a C++17 compiler, Git and Boost, as described in :ref:`build_from_source`. ecFlow's CMake does not
search the system paths for Boost; ``BOOST_ROOT`` must hold the Boost install prefix:

.. code-block:: shell

  BOOST_ROOT=/path/to/boost cargo build

A crate taken from the ecFlow repository builds the C++ sources of the same checkout. A crate taken from crates.io
clones the ecFlow release with the same version as the crate.

By default, the crate is built with OpenSSL support, based on the ``ssl`` feature. To build without OpenSSL,
disable the default features:

.. code-block:: toml

  [dependencies]
  ecflow = { version = "5.19", default-features = false }

Build Environment Variables
===========================

.. list-table::
   :header-rows: 1

   * - Environment variable
     - Default Value
     - Description
   * - BOOST_ROOT
     -
     - Boost install prefix
   * - ECBUILD_DIR
     - a clone of ecbuild
     - Path to an ecbuild checkout
   * - CMAKE_PREFIX_PATH
     -
     - Forwarded to CMake

Connecting to ecFlow server
===========================

``Client::new()`` expects the ecFlow server at the location given by the environment variables ``ECF_HOST`` and
``ECF_PORT``, as :term:`ecflow_client` does. ``Client::with_host_port()`` takes the location as arguments:

.. code-block:: rust

  use ecflow::Client;

  let mut client = Client::with_host_port("localhost", 3141)?;
  client.ping()?;
  println!("server {}", client.server_version()?);

Authentication
==============

The client sends the user name from ``ECF_USER``, or else the login name, with each request, and the ecFlow server
authenticates it with the available mechanisms (e.g. white list files, password based authentication).
``set_user_name()`` and ``set_password()`` override the user name and supply the password.

``enable_ssl()`` selects the SSL transport and locates the certificate as described in :ref:`open_ssl`.

API Documentation
=================

.. include:: rust_api/reference.inc

More Examples
=============

The examples are in ``rust/crates/ecflow/examples`` and run with ``cargo run --example <name>``.

Ping a server
-------------

.. literalinclude:: ../rust/crates/ecflow/examples/ping.rs
   :language: rust
   :lines: 4-

Load a suite definition
-----------------------

.. literalinclude:: ../rust/crates/ecflow/examples/defs.rs
   :language: rust
   :lines: 4-

Report the progress of a job
----------------------------

.. literalinclude:: ../rust/crates/ecflow/examples/child.rs
   :language: rust
   :lines: 4-
