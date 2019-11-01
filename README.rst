
**ECFLOW**
==========
ecFlow is a work-flow manager that enables users to run a large number of programs, 
with dependencies on each other and on time, in a controlled environment. 
It provides tolerance, for hardware and software failures, combined with 
good restart capabilities.

ecFlow runs as a server receiving requests from clients. 
The command line interface, the graphical interface, scripts and the 
Python API(application interface) are the clients. The server is based 
on C++/boost ASIO and uses TCP/IP for communication. 
Multiple servers can be run on the same hardware. 
ecFlow submits tasks (jobs) and receives acknowledgements from tasks via specific commands 
embedded in the scripts. The relationship between tasks is stored in ecFlow, 
and it is able to submit tasks dependent on the status of other tasks and attributes like time.

The command line interface for ecFlow allows the suite definition to be 
loaded and retrieved from the server. Also it provides a rich set of 
commands for communication with the server.

The Python API allows the entire suite definition structure to be specified 
and loaded into the server. A suite is a collection of interrelated tasks. 
In ecFlow suites are described by a definition file. 
The Python API also provides functionality for client to server communication. 
In addition, it allows checking of the suite, testing the defined 
interrelations between tasks, and other references and limits.

Documentation
-------------
ecFlow comes with a user manual, online tutorial, Python API and reference `documentation_
<https://confluence.ecmwf.int/display/ECFLOW/ecflow+home>`_. 


Install
-------
 
* dependencies

   - PYTHON_API:
     python 2.7,  Python 3.0  
     If you intend to use ecFlow Python api, You will need to install python.
     The python installation should include the development packages
     
   - ecflow_ui: ( new GUI )
     QT5 at least version 5.0.0 is required  
     
* ecfFlow consists of two tar files i.e. ::

   - boost_1_71_0.tar.gz
   - ecFlow-5.1.0-Source.tar.gz
   
* Create a directory for the build::
   .. code-block:: bash

    mkdir /tmp/ecflow_build
   
* Copy the the two tar file into this directory, then change directory to /tmp/ecflow_build
   
* Un-zip then un-tar the two file files::
   .. code-block:: bash

    tar -zxf boost_1_71_0.tar.gz
    tar -zxf ecFlow-5.1.0-Source.tar.gz
   
* You should have two directories created::
   .. code-block:: bash

     - boost_1_71_0
     - ecFlow-5.1.0-Source
    
* Create two environment variables. These are used by some of scripts::
   .. code-block:: bash

    export WK=/tmp/ecflow_build/ecFlow-5.1.0-Source
    export BOOST_ROOT=/tmp/ecflow_build/boost_1_71_0
   
* ecflow uses bjam to build BOOST libraries and cmake to build ecflow
  
* To maintain compatibility between different releases of ecflow, you
  should use the same version of boost. If you do not care about this,
  then any boost version (> 1.53) should suffice. To use an existing
  boost release please ensure environment variable BOOST_ROOT is set

boost libs
----------
Use the following step to build boost from scratch:

* Boost uses bjam for building the boost libraries. bjam source is available in boost, hence we first need to build bjam itself::
   .. code-block:: bash
  
    cd $BOOST_ROOT
    ./bootstrap.sh


  Now make sure bjam is accessible from $PATH

* Ecflow uses some of compiled libraries in boost. The following script will build the required lib's, and will configure boost build according to your platform::
   .. code-block:: bash
  
    cd $BOOST_ROOT
    $WK/build_scripts/boost_build.sh       # compile boost libs used by ecFlow


cmake
-----
* By default will install /usr/local, hence may require root access rights::
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.1.0-Source
    mkdir build; cd build
    cmake ..  
    make -j2
    make install
    make test 


* Optionally you can specify install prefix directory::
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.1.0-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/var/tmp/$USER/install/cmake/ecflow 
    make -j2
    make install


* Optionally if you do *NOT* want to build the GUI(ecflowview) or UI(ecflow_ui) or Python api::
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.1.0-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/var/tmp/$USER/install/cmake/ecflow \
             -DENABLE_UI=OFF            \
             -DENABLE_PYTHON=OFF
    make -j2
    make install


* Optionally if you did not export BOOST_ROOT you can specify on the command line::
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.1.0-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow \
             -DCMAKE_BUILD_TYPE=Debug \
             -DBOOST_ROOT=/tmp/ecflow_build/boost_1_71_0
    make -j2
    make install


* On some platforms(AIX) you may need to specify the c++ compiler::
   .. code-block:: bash
    
    cd /tmp/ecflow_build/ecFlow-5.1.0-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow \
                -DCMAKE_CXX_COMPILER=xlC_r"
    make -j2
    make install


* To use the python_api, you need to add/change PYTHONPATH and LD_LIBRARY_PATH::
   .. code-block:: bash

    export PYTHONPATH=$PYTHONPATH:$ECFLOW_PYTHON_INSTALL_DIR
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ECFLOW_PYTHON_INSTALL_DIR
      
      
Ecflow uses Semantic versioning
-------------------------------
See: http://semver.org/

