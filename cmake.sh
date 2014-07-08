#!/bin/sh

# ====================================================================
# Maximum of 1 arguments expected:
#  
if [ "$#" -gt 1 ] ; then
   echo "cmake expects 1 argument i.e"
   echo "  cmake.sh debug"
   echo "  cmake.sh release"
   exit 1
fi
if [[ "$1" != debug && "$1" != release  ]] ; then
  echo "cmake expected [ debug | release   ] but found $1"
  exit 1
fi

# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ===================================================================
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

mkdir -p ecbuild/$1
cd ecbuild/$1
      
cmake_build_type=
if [[ $1 = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

#
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
# -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#   *AND* for testing python install to local directory
#

cmake ../.. -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake \
            -DCMAKE_BUILD_TYPE=$cmake_build_type \
            -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$release.$major.$minor \
            -DCMAKE_PYTHON_INSTALL_TYPE=local \
            -DBOOST_ROOT=/var/tmp/ma0/boost/boost_1_53_0 \
            -DCMAKE_PYTHON_INSTALL_TYPE=local 
            #-DCMAKE_PYTHON_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$release.$major.$minor/lib/python2.7/site-packages/ecflow
        
