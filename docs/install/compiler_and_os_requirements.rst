.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _compiler_and_os_requirements:

Compiler and OS requirements
****************************

ecFlow supports Linux and macOS systems only.

:numref:`Table %s<operating_system_req_table>` shows the operating systems used to build and test ecFlow in CI, along with additional platforms in the castingbox build matrix. This list is broader than the distributions for which ECMWF provides packages.

.. list-table:: Operating systems used for CI builds and tests
   :header-rows: 1
   :widths: 20 40 40
   :name: operating_system_req_table

   * - Make
     - Compiler
     - Operating System
   * - Linux
     -

       * GNU GCC 8.5-15.x
       * LLVM Clang 18.x
     -

       * Alpine 3.24
       * AlmaLinux 9.8 and 10.2
       * Arch Linux (2026-09-23)
       * Debian 11.11, 12.15 and 13.7
       * Fedora 37 and 44
       * openSUSE Tumbleweed
       * Rocky Linux 8
       * Ubuntu 22.04, 24.04 and 26.04
   * - Darwin
     -

       * AppleClang 21.x
       * Homebrew LLVM Clang 19.x-23.x
     -

       * Ventura
       * Tahoe

Linux package builds
--------------------

ECMWF's package build pipeline currently builds packages for:

* Rocky Linux 8 (ecFlow UI)
* Debian 11, 12 and 13 (ecFlow server)

Other distributions listed above are covered by CI builds and tests, but ECMWF does not currently build packages for them.
