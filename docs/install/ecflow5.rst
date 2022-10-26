.. _ecflow5:

ecflow5
///////

Dependencies
============

-  cmake (   install cmake   (sudo apt-get install cmake)).   Need at
   least cmake/3.13.4

-  g++     (   install g++        (sudo apt-get install g++))   ecflow 5
   series can only be built with C++14 or later  compilers. i.e. at the
   minimum gcc 6.3 or clang 6.0.

-  Python 3(optional)

   If you intend to use *ecFlow* Python API, You will need to install
   Python. (install **python-dev** (sudo apt-get install
   **python-dev**))

   Please ensure that *python* is accessible on $PATH otherwise, you may
   need to customise *$BOOST_ROOT/tools/build/v2/site-config.jam . *

   | The python installation **should** include the **development**
     packages
   | If you do not need the python API, then you can build without it,
     see below.

-  | Qt for
     `ecFlowUI <https://confluence.ecmwf.int/display/ECFLOW/ecFlowUI+Documentation>`__
     (Qt5 mandatory).
   | For self-installed Qt libraries, consider setting CMAKE_PREFIX_PATH
     (see below). See also http://doc.qt.io/qt-5/cmake-manual.html for
     further details.

-  OpenSSL(optional)

-  sphinx(optional), this is used to generate ecflow python API

Setting up the build environment
================================

-  ecFlow consists of two tar files i.e.:  

   -  boost_1_71_0.tar.gz (or any recent boost)

   -  ecFlow-5.8.1-Source.tar.gz

      Create a directory for the build:

.. note::

 mkdir /tmp/ecflow_build                                               

-  Copy the two tar file into this directory, then change directory to
   **/tmp/ecflow_build**

-  Un-zip then un-tar the two file files:

.. note::

 tar -zxf boost_1_71_0.tar.gz                                          
                                                                       
 tar -zxf ecFlow-5.8.1-Source.tar.gz                                   

-  You should have two directories created:

.. note::

 boost_1_71_0                                                          
                                                                       
 ecFlow-5.8.1-Source                                                   

-  Create two environment variables. These are used by some of the
   scripts:

.. note::

 export WK=/tmp/ecflow_build/ecFlow-5.8.1-Source                       
                                                                       
 export BOOST_ROOT=/tmp/ecflow_build/boost_1_71_0                      

-  If you have a module system, please ensure that before you start,
   GCC,cmake,python3, etc are available in $PATH.

.. note::

 module load gnu                                                       
                                                                       
 module load cmake                                                     
                                                                       
 module load python3                                                   
                                                                       
 module load qt                                                        

Build boost
===========

In future releases of ecflow 5 series, the client/server versions of
ecflow do **not** have to be built with the same boost version. This is
because ecflow 5  now uses JSON for client/server communication.

-  | Boost uses bjam/b2 for building the boost libs.
   | bjam source is available in boost, hence we first need to build
     bjam itself:

.. note::

 cd $BOOST_ROOT                                                        
                                                                       
 ./bootstrap.sh                                                        

-  If you do not require the ecFlow python API, you can avoid building
   boost python libs by setting

.. note::

 export ECF_NO_PYTHON=1                                                

Code Block 1 Disable boost python, if ecflow PYTHON api not required

**            You will also need to disable python when building
ecFlow.  See the instruction under cmake **\ before calling
$WK/build_scripts/boost_build.sh (see below)

-  ecFlow uses some of the compiled libraries in boost. The following
   script will build the required lib’s and configure boost build
   according to your platform. 

.. note::

 cd $BOOST_ROOT                                                        
                                                                       
 $WK/build_scripts/boost_build.sh # compile boost libs used by ecFlow. 
 Please see notes in boost_build.sh, if you want to build both for     
 python2 and python3                                                   

Code Block 2 Build boost libraries including python3 used by ecflow.

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

+-----------------------+-------------------------------------+-------+
| cmake options         | doc                                 | de    |
|                       |                                     | fault |
+=======================+=====================================+=======+
| CMAKE_INSTALL_PREFIX  | where you want to install your      |       |
|                       | ecFlow                              | /usr/ |
|                       |                                     | local |
+-----------------------+-------------------------------------+-------+
| CMAKE_BUILD_TYPE      | to select the type of compilation:  | Re    |
|                       |                                     | lease |
|                       | -  Debug                            |       |
|                       |                                     |       |
|                       | -  RelWithDebInfo                   |       |
|                       |                                     |       |
|                       | -  Release (fully optimised         |       |
|                       |       compiler options)             |       |
|                       |                                     |       |
|                       | -  Production                       |       |
+-----------------------+-------------------------------------+-------+
| CMAKE_CXX_FLAGS       | more flags  for the C++ compiler    |       |
+-----------------------+-------------------------------------+-------+
| ENABLE_SERVER         | build the ecFlow server             | on    |
+-----------------------+-------------------------------------+-------+
| ENABLE_PYTHON         | enable python interface             | on    |
+-----------------------+-------------------------------------+-------+
| ENABLE_UI             | enable the build of ecflowUI        | on    |
|                       | (requires Qt)                       |       |
+-----------------------+-------------------------------------+-------+
| CMAKE_PREFIX_PATH     | use to provide a path to dependent  |       |
|                       | libraries that are installed in     |       |
|                       | non-system locations.               |       |
|                       | For example, if you have installed  |       |
|                       | Qt in a non-system location, you    |       |
|                       | should set the path in this         |       |
|                       | variable.                           |       |
+-----------------------+-------------------------------------+-------+
| ENABLE_ALL_TESTS      | enable performance, migration,      | off   |
|                       | memory leak, and regression tests   |       |
+-----------------------+-------------------------------------+-------+
| ENABLE_SSL            | Encrypted communication for user    | on    |
|                       | commands                            |       |
|                       |                                     |       |
|                       | Please see: `Open                   |       |
|                       | SSL <https://confluence.ec          |       |
|                       | mwf.int/display/ECFLOW/Open+ssl>`__ |       |
|                       | for more details.                   |       |
+-----------------------+-------------------------------------+-------+
| BOOST_ROOT            | where to find boost ( if            |       |
|                       | non-standard installation  )        |       |
|                       |                                     |       |
|                       | If not specified cmake will look    |       |
|                       | for an environment variable of the  |       |
|                       | same name.                          |       |
+-----------------------+-------------------------------------+-------+
| ENA                   | By default we build with static     | on    |
| BLE_STATIC_BOOST_LIBS | boost libs, to use shared boost     |       |
|                       | list set to OFF                     |       |
+-----------------------+-------------------------------------+-------+

The  C++  compilers are chosen by CMake. (This can be overwritten by
setting the environment variables *CXX* on the command line before you
call *cmake*, to the preferred compiler).

Further, the variable *CMAKE_CXX_FLAGS* can be used to set compiler
flags for optimisation or debugging. 

+-----------------------------------------------------------------------+
|    cd $WK                                                             |
|                                                                       |
|    mkdir build; cd build;                                             |
|                                                                       |
|                                                                       |
|                                                                       |
|    # Go with defaults, will build with CMAKE_BUILD_TYPE=Release and   |
|    install to /usr/local                                              |
|                                                                       |
|    cmake ..                                                           |
|                                                                       |
|    # Override install prefix                                          |
|                                                                       |
|    # cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow/5.8.1     |
|                                                                       |
|                                                                       |
|                                                                       |
|    # do NOT build the gui.                                            |
|                                                                       |
|    # cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow           |
|    -DCMAKE_BUILD_TYPE=Release -DENABLE_UI=OFF                         |
|                                                                       |
|                                                                       |
|                                                                       |
|    # ignore Wdeprecated-declarations compiler warning messages and do |
|    NOT build python api                                               |
|                                                                       |
|    # cmake .. -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"        |
|    -DENABLE_PYTHON=OFF                                                |
|                                                                       |
|                                                                       |
|                                                                       |
|    # Use -j option to speed up compilation. Determine number of cpu's |
|                                                                       |
|    CPUS=$(lscpu -p \| grep -v '#' \| wc -l)                           |
|                                                                       |
|    make -j${CPUS}                                                     |
|                                                                       |
|    make check                                                         |
|                                                                       |
|    make install                                                       |
+=======================================================================+
+-----------------------------------------------------------------------+

Code Block 3 cmake/ecbuild

+-----------------------------------------------------------------------+
|    If you experience a problem with your installation and need to fix |
|    your install of dependent libraries like QT, Python, Boost, GCC,   |
|    etc,  then it is **VERY** important that you **delete** the build  |
|    directory and start cmake build again. (This is because cmake      |
|    keeps a cache of your configuration, and re-uses this unless the   |
|    build directory is deleted).                                       |
|                                                                       |
| +------------------------------------------------------------------+  |
| |    cd $WK                                                        |  |
| |                                                                  |  |
| |    rm -rf build                                                  |  |
| |                                                                  |  |
| |    mkdir build; cd build                                         |  |
| |                                                                  |  |
| |    cmake .. # or use whatever cmake configuration you used       |  |
| |    before                                                        |  |
| +==================================================================+  |
| +------------------------------------------------------------------+  |
|                                                                       |
| ..                                                                    |
|                                                                       |
|    Code Block 4 Always remember to delete build directory if there is |
|    a change in system configuration                                   |
+=======================================================================+
+-----------------------------------------------------------------------+

To use the `ecFlow Python
Api <https://confluence.ecmwf.int/display/ECFLOW/ecFlow+Python+Api#python-api>`__,
you need to add/change PYTHONPATH. 

.. note::

    export                                                             
    PYT                                                                
 HONPATH=$PYTHONPATH:<prefix>/5.8.1/lib/python3.6/site-packages/ecflow 
                                                                       
    # If you used the default's then <prefix>=/usr/local               
                                                                       
    # otherwise you should use whatever you entered for                
    -DCMAKE_INSTALL_PREFIX, hence in the examples above we would have: 
                                                                       
    export                                                             
    PYTHONPATH=$PYTHON                                                 
 PATH:/usr/local/apps/ecflow/5.8.1/lib/python3.6/site-packages/ecflow  

Installing ecFlow Python to a custom  directory
-----------------------------------------------

The default install for ecFlow will install python(if it was enabled)
under the directory given to CMAKE_INSTALL_PREFIX.

However, sometimes we may need to install the ecFlow python module to a
different prefix.

This can be done using:

.. note::

    cd $WK/build # change to the build directory                       
                                                                       
    cmake -DCMAKE_INSTALL_PREFIX=/tmp/avi/custom/ecflow/5.8.1          
    -DCOMPONENT=python -P cmake_install.cmake -- make install #        
    install python module under /tmp/avi/custom/ecflow/5.8.1           

ecflow_ui: Make a list servers accessible to all users
------------------------------------------------------

The GUI used by ecFlow is called
`ecflow_ui <https://confluence.ecmwf.int/display/ECFLOW/ecFlowUI>`__. 
This is used to interact and visualize the ecFlow servers.

You can make the list of servers available for your users by:

-  creating a file called **servers**

-  The format of the server's file is very easy:

.. note::

 <server_name> <machine_name> <port>                                   

Code Block 5 server file format

   An example might be:

.. note::

 server machineX 3141                                                  
                                                                       
 projectX machineabc 4141                                              
                                                                       
 exp1 machineabc 4141                                                  
                                                                       
 mars bigmac 11031                                                     

Code Block 6 servers file

-  Copy this file to CMAKE_INSTALL_PREFIX/share/ecflow/.   This makes
   the list of servers accessible to all users of ecflow_ui

.. note::

 cp servers /tmp/avi/custom/ecflow/5.8.1/share/ecflow/.                

Python API (from ecflow 5.6.0)
------------------------------

The most up to date python API documentation can be generated. This
requires that **sphinx-build** is available on $PATH.

.. note::

    This step relies on build being complete .i.e                      
    `ecflow.so <http://ecflow.so>`__ must have been built or installed 

.. note::

    cd $WK                                                             
                                                                       
    cd Doc/online                                                      
                                                                       
    make clean; make html                                              

The api can then be viewed in the browser. Please open file
$WK/Doc/online/_build/html/index.html
