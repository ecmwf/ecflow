.. _build_from_source:

Build from source
/////////////////////

Dependencies
============

-  cmake: need at least cmake/3.13.4

-  g++: can only be built with C++14 or later compilers. i.e. at the
   minimum gcc 6.3 or clang 6.0.

-  Python 3 (optional): ff you intend to use *ecFlow* :ref:`python_api`. You will need to install
   Python (install **python-dev** (sudo apt-get install **python-dev**)).

   Please ensure that *python* is accessible on $PATH otherwise, you may
   need to customise *$BOOST_ROOT/tools/build/v2/site-config.jam.*

   The python installation **should** include the **development**
   packages If you do not need the python API, then you can build without it, see below.

-  Qt for :ref:`ecflow_ui` (Qt5 or Qt6 mandatory). For self-installed Qt libraries, consider setting CMAKE_PREFIX_PATH (see below). See also http://doc.qt.io/qt-5/cmake-manual.html for further details.

-  OpenSSL (optional)

Setting up the build environment
================================

-  ecFlow consists of two tar files i.e.:

   -  boost_1_71_0.tar.gz (or any recent boost)
   -  ecFlow-5.8.1-Source.tar.gz


   Create a directory for the build:

   .. code-block:: shell

      mkdir /tmp/ecflow_build                                               

-  Copy the two tar file into this directory, then change directory to **/tmp/ecflow_build**

-  Un-zip then un-tar the two file files:

   .. code-block:: shell

      tar -zxf boost_1_71_0.tar.gz
      tar -zxf ecFlow-5.8.1-Source.tar.gz                                   

-  You should have two directories created:

   .. code-block:: shell
 
      boost_1_71_0
      ecFlow-5.8.1-Source                                                   

-  Create two environment variables. These are used by some of the
   scripts:

   .. code-block:: shell

      export WK=/tmp/ecflow_build/ecFlow-5.8.1-Source
      export BOOST_ROOT=/tmp/ecflow_build/boost_1_71_0                      

-  If you have a module system, please ensure that before you start,
   GCC,cmake,python3, etc are available in $PATH.

   .. code-block:: shell
         
      module load gnu
      module load cmake
      module load python3
      module load qt                                                     

Build boost
===========

In future releases of ecflow 5 series, the client/server versions of
ecflow do **not** have to be built with the same boost version. This is
because ecflow 5 now uses JSON for client/server communication.

-  Boost uses bjam/b2 for building the boost libs. bjam source is available in boost, hence we first need to build bjam itself:

   .. code-block:: shell

      cd $BOOST_ROOT
      ./bootstrap.sh                                                        

-  If you do not require the ecFlow :ref:`python_api`, you can avoid building boost python libs by setting:

   .. code-block:: shell
      :caption: Disable boost python, if ecflow PYTHON api not required

      export ECF_NO_PYTHON=1                                                

   **You will also need to disable python when building ecFlow. See the instruction under cmake** before calling $WK/build_scripts/boost_build.sh (see below)

-  ecFlow uses some of the compiled libraries in boost. The following
   script will build the required lib’s and configure boost build
   according to your platform.

   .. code-block:: shell
      :caption:  Build boost libraries including python3 used by ecflow

      cd $BOOST_ROOT
      $WK/build_scripts/boost_build.sh # compile boost libs used by ecFlow. Please see notes in boost_build.sh, if you want to build both for python2 and python3                                                   

Build
=====

cmake
-----

As configure, CMake will run some tests on the customer's system to find
out if required third-party software libraries are available and note
their locations (paths). Based on this information it will produce the
Makefiles needed to compile and install ecFlow

**CMake** is a cross-platform free software program for managing the
build process of software using a compiler-independent method.

Generating the Makefiles with CMake
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After changing into the build ecflow directory, the user has to run
CMake with his/her own options. The command gives feedback on what
requirements are fulfilled and what software is still required. Table
below gives an overview of the different options of configure.  The
default (without any options) will install in /usr/local/.


.. list-table::
   :header-rows: 1

   * - cmake options
     - doc
     - default
   * - CMAKE_INSTALL_PREFIX
     - where you want to install your ecFlow
     - /usr/local
   * - CMAKE_BUILD_TYPE
     - to select the type of compilation:
  
       - Debug
       - RelWithDebInfo
       - Release (fully optimised compiler options)
       - Production
     - Release
   * - CMAKE_CXX_FLAGS
     - more flags for the C++ compiler
     -
   * - ENABLE_SERVER
     - build the ecFlow server
     - on
   * - ENABLE_PYTHON
     - enable python interface
     - on
   * - ENABLE_UI
     - enable the build of ecflowUI (requires Qt)
     - on
   * - CMAKE_PREFIX_PATH
     - use to provide a path to dependent libraries that are installed in non-system locations.For example, if you have installed Qt in a non-system location, you should set the path in this variable.
     -
   * - ENABLE_ALL_TESTS
     - enable performance, migration, memory leak, and regression tests
     - off
   * - ENABLE_SSL
     - Encrypted communication for user commands. Please see: :ref:`open_ssl` for more details.
     - on
   * - BOOST_ROOT
     - where to find boost (if non-standard installation). If not specified cmake will look for an environment variable of the same name.
     -
   * - ENABLE_STATIC_BOOST_LIBS
     - By default we build with static boost libs, to use shared boost list set to OFF
     - on

The C++ compilers are chosen by CMake. (This can be overwritten by
setting the environment variables *CXX* on the command line before you
call *cmake*, to the preferred compiler).

Further, the variable *CMAKE_CXX_FLAGS* can be used to set compiler
flags for optimisation or debugging.

.. code-block:: shell
   :caption: cmake/ecbuild

   cd $WK
   mkdir build; cd build;
   
   # Go with defaults, will build with CMAKE_BUILD_TYPE=Release and install to /usr/local
   cmake ..
   # Override install prefix
   # cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow/5.8.1
   
   # do NOT build the gui.
   # cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow -DCMAKE_BUILD_TYPE=Release -DENABLE_UI=OFF
   
   # ignore Wdeprecated-declarations compiler warning messages and do NOT build python api
   # cmake .. -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"  -DENABLE_PYTHON=OFF
   
   # Use -j option to speed up compilation. Determine number of cpu's
   CPUS=$(lscpu -p | grep -v '#' | wc -l)
   make -j${CPUS}
   make check
   make install   

.. warning:: 

   If you experience a problem with your installation and need to fix your install of dependent libraries like QT, Python, Boost, GCC, etc,  then it is VERY important that you delete the build directory and start cmake build again. (This is because cmake keeps a cache of your configuration, and re-uses this unless the build directory is deleted).

   .. code-block:: shell
      :caption: Always remember to delete build directory if there is a change in system configuration

      cd $WK
      rm -rf build
      mkdir build; cd build
      cmake ..      # or use whatever cmake configuration you used before

To use the ecFlow :ref:`python_api` you need to add/change PYTHONPATH. 

.. code-block:: shell

   export PYTHONPATH=$PYTHONPATH:<prefix>/5.8.1/lib/python3.6/site-packages/ecflow
   # If you used the default's then <prefix>=/usr/local
   # otherwise you should use whatever you entered for -DCMAKE_INSTALL_PREFIX, hence in the examples above we would have:
   export PYTHONPATH=$PYTHONPATH:/usr/local/apps/ecflow/5.8.1/lib/python3.6/site-packages/ecflow 

Installing ecFlow Python to a custom  directory
-----------------------------------------------

The default install for ecFlow will install python (if it was enabled)
under the directory given to CMAKE_INSTALL_PREFIX.

However, sometimes we may need to install the ecFlow python module to a
different prefix.

This can be done using:

.. code-block:: shell

   cd $WK/build  # change to the build directory
   cmake -DCMAKE_INSTALL_PREFIX=/tmp/avi/custom/ecflow/5.8.1 -DCOMPONENT=python -P cmake_install.cmake -- make install  # install python module under /tmp/avi/custom/ecflow/5.8.1
   

ecflow_ui: Make a list servers accessible to all users
------------------------------------------------------

The GUI used by ecFlow is called :ref:`ecflow_ui`. This is used to interact and visualize the ecFlow servers.

You can make the list of servers available for your users by:

-  creating a file called **servers**

-  The format of the server's file is very easy:

   .. code-block:: shell
      :caption: server file format

      <server_name> <machine_name> <port>                                   

   .. code-block:: shell
      :caption:  servers file

      server      machineX   3141
      projectX    machineabc 4141
      exp1        machineabc 4141
      mars        bigmac     11031

-  Copy this file to CMAKE_INSTALL_PREFIX/share/ecflow/.   This makes
   the list of servers accessible to all users of ecflow_ui

   .. code-block:: shell

      cp servers /tmp/avi/custom/ecflow/5.8.1/share/ecflow/.                
