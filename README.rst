**ecFlow**
==========

.. contents:: Table of Contents


Overview
--------

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


Contributing
------------

Code contributions on ECMWF GitHub space should follow the standard fork-based contribution model on GitHub, which ends with opening of a pull request. 
Any contribution should follow these steps:

- Fork the develop branch of the targeted package from GitHub to your own GitHub space
- Clone your fork locally
- Make the necessary code changes & add and run tests to ensure the new codes works as intended
- Push changes back to fork on GitHub
- Create a pull request (PR) back to ECMWF:
   * Describe the motivation of the change and impact on code
   * Accept the ECMWF Contributors License Agreement (CLA - see below for more information)
   * Make sure that all requirements of the PR are addressed
- As soon as all conditions are fulfilled an ECMWF staff member will review the PR and either merge the request or comment on the PR

Also see License_



Issues
------

Any issues with ECMWF software should continue to be reported either by email 
to Software.Software@ecmwf.int or in the Software Support page at ECMWF. 

The issue management system on GitHub should only be used for issues around code contributions to the packages. 
If in doubt please contact ECMWF Software Support.

Prebuilt binaries
-----------------

Prebuilt binaries are available for most linix `distributions. <https://confluence.ecmwf.int/display/ECFLOW/Packages>`_


Install from source
-------------------
 
* dependencies::

   - GNU C++ (or CLANG) compiler
   - PYTHON_API(optional): python 2.7,  Python 3.0  
     If you intend to use ecFlow Python api, You will need to install python.
     The python installation should include the development packages
   - QT5 at least version 5.0.0 is required (optional)
   - cmake
   - Boost C++ libraries
   - Openssl(optional)

* ecfFlow consists of two tar files i.e. ::

   - boost_1_71_0.tar.gz
   - ecFlow-5.8.4-Source.tar.gz

* Create a directory for the build
   .. code-block:: bash

    mkdir /tmp/ecflow_build

* Copy the two tar file into this directory, then change directory to /tmp/ecflow_build
   
* Un-zip then un-tar the two files
   .. code-block:: bash

    tar -zxf boost_1_71_0.tar.gz
    tar -zxf ecFlow-5.8.4-Source.tar.gz

* You should have two directories created
   .. code-block:: bash

    - boost_1_71_0
    - ecFlow-5.8.4-Source

* Create two environment variables. These are used by some of scripts
   .. code-block:: bash

    export WK=/tmp/ecflow_build/ecFlow-5.8.4-Source
    export BOOST_ROOT=/tmp/ecflow_build/boost_1_71_0

* ecflow uses bjam/b2 to build BOOST libraries and cmake to build ecflow
  
* To maintain compatibility between different releases of ecflow, you
  should use the same version of boost. If you do not care about this,
  then any boost version (> 1.68) should suffice. To use an existing
  boost release please ensure environment variable BOOST_ROOT is set

boost libs
^^^^^^^^^^
Use the following step to build boost from scratch:

* Boost uses bjam for building the boost libraries. bjam source is available in boost, hence we first need to build bjam itself
   .. code-block:: bash

    cd $BOOST_ROOT
    ./bootstrap.sh


  Now make sure bjam is accessible from $PATH

* Ecflow uses some of compiled libraries in boost. The following script will build the required lib's, and will configure boost build according to your platform
   .. code-block:: bash

    cd $BOOST_ROOT
    $WK/build_scripts/boost_build.sh       # compile boost libs used by ecFlow


cmake
^^^^^
* By default will install /usr/local, hence may require root access rights
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.8.4-Source
    mkdir build; cd build
    cmake ..  
    make -j2
    make install
    make test 

* Optionally you can specify install prefix directory
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.8.4-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/var/tmp/$USER/install/cmake/ecflow 
    make -j2
    make install

* Optionally if you do *NOT* want to build the UI(ecflow_ui) or Python api
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.8.4-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/var/tmp/$USER/install/cmake/ecflow \
             -DENABLE_UI=OFF \
             -DENABLE_PYTHON=OFF
    make -j2
    make install

* Optionally if you did not export BOOST_ROOT you can specify on the command line
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.8.4-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow \
             -DCMAKE_BUILD_TYPE=Debug \
             -DBOOST_ROOT=/tmp/ecflow_build/boost_1_71_0
    make -j2
    make install

* On some platforms(AIX) you may need to specify the c++ compiler
   .. code-block:: bash

    cd /tmp/ecflow_build/ecFlow-5.8.4-Source
    mkdir build; cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow \
                -DCMAKE_CXX_COMPILER=xlC_r"
    make -j2
    make install


* To use the python_api, you need to add/change PYTHONPATH and LD_LIBRARY_PATH
   .. code-block:: Bash

     export PYTHONPATH=$PYTHONPATH:$ECFLOW_PYTHON_INSTALL_DIR
     export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ECFLOW_PYTHON_INSTALL_DIR

Cereal
^^^^^^
* ecflow uses embedded CEREAL(version 1.3.0) for client/server communication via JSON


Python Api
^^^^^^^^^^
* The ecflow python API can be generated using sphinx. To do this ensure that *sphinx-build* is available on the path.

* You have built ecflow using cmake, then please do the following
   .. code-block:: bash

    cd $WK
    cd Doc/online
    make clean
    make html
    
* The documentation can then be viewed using yout browser. 
  Just open the file $WK/Doc/online/_build/html/index.html

.. License:

License
-------

All open source software packages at ECMWF are distributed under the Apache License 2.0. 
The standard Apache License was amended to cater for ECMWF special status as an international organisation. 
For you to be able to contribute any code to our software packages you need to agree that

- you are happy for your code to be redistributed under Apache License.
- that your contribution does not violate anyone's IPR rights.

To do so, you and your organisation need to sign a contributors agreement.


Support/Contact
---------------

Please see: `ECMWF Support page <https://confluence.ecmwf.int/display/SUP/Support>`_

Email contacts:

- software.support@ecmwf.int 

