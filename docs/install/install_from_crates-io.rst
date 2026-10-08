.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _crates-io:

Install from crates.io
**********************

The ecFlow :ref:`rust_api` is available as the ``ecflow`` crate from crates.io: https://crates.io/crates/ecflow

Add it to the dependencies of a Rust project:

.. code-block:: shell

    cargo add ecflow

Alternatively, take the crate from the ecFlow repository on GitHub, at a branch, tag or revision:

.. code-block:: shell

    cargo add ecflow --git https://github.com/ecmwf/ecflow --branch develop

The crate builds the ecFlow client library from source, so the build needs CMake, a C++17 compiler, Git and
Boost, with ``BOOST_ROOT`` pointing at the Boost install prefix. See the :ref:`Rust API <rust_api>` for the
details of the build.
