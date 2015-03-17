#!/bin/sh

# ======================================================================
# Use for install of ecflow using cmake and ecbuild(bundled with ecflow)

# =====================================================================
# Maximum of 1 arguments expected:
if [ "$#" -gt 1 ] ; then
   echo "cmake expects 1 argument i.e"
   echo "  ecbuild.sh debug"
   echo "  ecbuild.sh release"
   exit 1
fi
if [[ "$1" != debug && "$1" != release  ]] ; then
  echo "ecbuild.sh expected [ debug | release   ] but found $1"
  exit 1
fi

# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ===================================================================
# Version number
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

# ===================================================================
# Build directory
cd ..
mkdir -p build_dir/$1
cd build_dir/$1
      
cmake_build_type=
if [[ $1 = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

# ===================================================================
# ecbuild/cmake

# BOOST_ROOT:  
#  By default it looks for environment variable BOOST_ROOT, if not it can specified on the command line. i.e
#  -DBOOST_ROOT=/var/tmp/ma0/boost/boost_1_53_0

# PREFIX: 
#   This needs to be set to path where you want to install
INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow

# Python:
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
#    local : this will install to $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory

cmake $WK \
      -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake \
      -DCMAKE_BUILD_TYPE=$cmake_build_type \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX/$release.$major.$minor \
      -DCMAKE_PYTHON_INSTALL_TYPE=local 
      #-DCMAKE_PYTHON_INSTALL_PREFIX=$INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow
     
# ===================================================================   
# next steps:
# make -j2
# umask 0022
# make install