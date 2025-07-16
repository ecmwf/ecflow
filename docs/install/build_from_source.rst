.. _build_from_source:

Build from source
///////////////////////////////////////////////////////////////////////////////

Build Environment
===============================================================================

- Git

- Make or Ninja

- CMake

   - CMake 3.18+

- C++ Compiler

   - C++17 support is required
   - GNU GCC 8.5+ or Clang 14.0+

- ecbuild

   - ecbuild 3.11+

- Boost

   - Boost 1.69+
   - mandatory components: system, timer, filesystem, program_options, chrono,
     date_time, unit_test_framework
   - optional components: python (required only by ecFlow :ref:`python_api`)

- Python 3 (optional)

   - Python 3.8+
   - required only by ecFlow :ref:`python_api`
   - must include development artifacts (i.e. include directory and library)

- Qt (optional)

   - Qt 5+ or Qt 6+
   - mandatory components: qtgui, qtwidgets, qtnetwork, qtsvg
   - optional components: qtcharts
   - required only by :ref:`ecflow_ui`.

- OpenSSL (optional)

- Sphinx & Doxygen (optional)

   - required only when building ecFlow documentation

.. note::
   The installation procedure of the above dependencies is specific to each
   target platform, and beyond the scope of this user manual.

   In case the target platform is a Linux machine, the dependencies might be
   installed using the applicable package manager (e.g. ``yum``, ``dnf`` or
   ``apt``), and on Mac machines consider using Homebrew or conda-forge for
   package management. On an HPC environment, most likely there is an
   Environment Modules system that enables loading the necessary dependencies.

   Please refer to your intended target platform documentation for help.

Build using CMake
===============================================================================

**CMake** is a cross-platform free software program for managing the build
process of software using a compiler-independent method. The following sections
describe how CMake is used to build and install ecFlow.

Create a development directory
-------------------------------------------------------------------------------

Create a new *development directory* as follows

.. code-block:: shell

   mkdir -p /path/to/development
   cd /path/to/development


Get the source code
-------------------------------------------------------------------------------

The ecFlow source code repository is available at `github <https://github.com/ecmwf/ecflow>`_.
Use Git to clone the source code repository as follows

.. code-block:: shell

   # using https
   git clone https://github.com/ecmwf/ecflow.git
   # using ssh
   git clone git@github.com:ecmwf/ecflow.git

.. warning::
   The cloned repository will have the default content i.e. the content of branch `develop`.

ecFlow source code release packages can also be found at
`releases <https://confluence.ecmwf.int/display/ECFLOW/Releases>`_.
Download the intended release package into the development directory,
and extract the source code as follows

.. code-block:: shell

   tar zxvf ecFlow-<version>-Source.tar.gz
   mv ecFlow-<version>-Source ecflow

Regardless of using Git to clone the repository, or downloading and extracting
the source code package, ecFlow source code should now be available at the
*source directory* (``/path/to/development/ecflow``).

.. note::

   The ecFlow build depends also on `*ecbuild* <https://github.com/ecmwf/ecbuild>`_.

   While *ecbuild* can be installed in the system, the easiest way to setup
   the development environment is usually to use Git to clone the *ecbuild*
   repository, and checkout the latest source code version, as follows

   .. code-block:: shell

      # using https
      git clone https://github.com/ecmwf/ecbuild.git
      # using ssh
      git clone git@github.com:ecmwf/ecbuild.git

      # checkout ecbuild 3.4.0 or later
      cd ecbuild
      git checkout -b use_for_ecflow tags/3.4.0
      cd ..

Configure ecFlow
-------------------------------------------------------------------------------

CMake examines the development environment configuration to find the location
of required software tools and libraries, and produces the build scripts to
compile and install ecFlow. This step is called project configuration.

The ecFlow build is configured *out-of-source*, meaning that the build scripts
are generated in a separate directory from the source code.

To configure ecFlow, using the default configuration, run the following
commands inside the *source directory* (``/path/to/development/ecflow``).
The directory that stores the generated build scripts is usually called
*build directory* (``/path/to/development/ecflow/build``).

.. code-block:: shell

   # store the build scripts in the ``build`` directory
   cmake -B build -S .

The following table shows an overview of useful CMake options.

.. list-table::
   :header-rows: 1

   * - CMake Option
     - Description
     - Default Value
   * - CMAKE_INSTALL_PREFIX
     - install location for ecFlow
     - /usr/local/ecflow
   * - CMAKE_BUILD_TYPE
     - supported compilation types: Debug, RelWithDebInfo, Release
     - Release
   * - CMAKE_CXX_COMPILER
     - location of the C++ compiler to use
     -
   * - CMAKE_CXX_FLAGS
     - custom C++ compiler flags
     -
   * - ENABLE_SERVER
     - build the ecFlow server
     - ON
   * - ENABLE_PYTHON
     - enable ecFlow Python interface
     - ON
   * - ENABLE_UI
     - enable ecFlowUI
     - ON
   * - ENABLE_HTTP
     - enable ecFlow REST API
     - ON
   * - ENABLE_UDP
     - enable ecFlow UDP API
     - ON
   * - CMAKE_PREFIX_PATH
     - use to provide a path to dependent libraries that are installed in non-system locations.For example, if you have installed Qt in a non-system location, you should set the path in this variable.
     -
   * - ENABLE_TESTS
     - enable unit tests
     - ON
   * - ENABLE_ALL_TESTS
     - enable additional performance, migration, memory leak, and regression tests
     - OFF
   * - ENABLE_SSL
     - enable encrypted communication between server and client -- see: :ref:`open_ssl` for more details
     - ON
   * - BOOST_ROOT
     - location of Boost library
     -
   * - ENABLE_STATIC_BOOST_LIBS
     - allow using static or dynamic Boost libraries (static are used by default)
     - ON

CMake generates Unix Makefiles by default, but Ninja build files can also be used.
To generate Ninja build files, consider the following example command.

.. code-block:: shell

   cmake -G Ninja -B build -S .

To disable specific ecFlow components, such as ecFlow Python and ecFlow UI,
consider the following example command.

.. code-block:: shell

   cmake -B build -S . -DENABLE_PYTHON=OFF -DENABLE_UI=OFF

To configure a specific ecFlow install directory, consider the following example command.

.. code-block:: shell

   cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/path/to/install/ecflow

To configure the use of a custom compiler, consider the following example command.

.. code-block:: shell

   cmake -B build -S . -DCMAKE_CXX_COMPILER=/path/to/custom/compiler

To configure the use of custom compiler flags, consider the following example command.

.. code-block:: shell

   cmake -B build -S . -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"

To configure the use of 3rd party libraries installed in custom locations,
consider the following example command.

.. code-block:: shell

   cmake -B build -S . \
         -DBOOST_ROOT=/path/to/boost \
         -DPython3_EXECUTABLE=/path/to/python/bin/python3 \
         -DCMAKE_PREFIX_PATH=/path/to/qt/lib/cmake

.. warning::

   When experiencing issues related to the location of 3rd party dependencies
   during project configuration, consider deleting the build directory and
   restart the configuration process again.

   This is sometimes necessary because CMake caches configuration information,
   and re-uses it unless the build directory is deleted.


Build ecFlow
-------------------------------------------------------------------------------

Once successfully configured, ecFlow is built using the following command
(the ``-j`` option allows to run multiple compilation tasks in parallel).

.. code-block:: shell

   cmake --build build -j 8

To build a specific artifact, such as ``ecflow_client``, consider the following command.

.. code-block:: shell

   cmake --build build -j 8 --target ecflow_client


(Optional) Test ecFlow
-------------------------------------------------------------------------------

Once successfully built, ecFlow tests are executed using the following command.

.. code-block:: shell

   ctest --test-dir build


Install ecFlow
-------------------------------------------------------------------------------

Finally, to install ecFlow use the following command.

.. code-block:: shell

   cmake --build build --target install

.. warning::

   Don't forget to define the installation directory using ``CMAKE_INSTALL_PREFIX``,
   during the `Configure ecFlow`_ step.

The default ecFlow installation step deploys the Python API under the directory
defined by ``CMAKE_INSTALL_PREFIX``. To install the Python API at a different
location, use the following commands.

.. code-block:: shell

   cd /path/to/development/ecflow/build  # change to the build directory

   cmake -DCMAKE_INSTALL_PREFIX=/path/to/ecflow/python \
         -DCOMPONENT=python \
         -P cmake_install.cmake

.. note::

   To use the ecFlow :ref:`python_api` you need to update ``PYTHONPATH``.

   .. code-block:: shell

      export PYTHONPATH=$PYTHONPATH:<prefix>/lib/python<version>/site-packages/ecflow
      # where <prefix> is the custom location defined by CMAKE_INSTALL_PREFIX,
      # or the default location /usr/local/ecflow in case no custom location is defined

      # in the example, use the following
      export PYTHONPATH=$PYTHONPATH:/path/to/ecflow/python/lib/python<version>/site-packages/ecflow
