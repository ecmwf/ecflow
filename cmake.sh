#!/bin/sh

# ====================================================================
# Maximum of 1 arguments expected:
#  
if [ "$#" -gt 1 ] ; then
   echo "cmake expects 1 argument i.e"
   echo "  cmake debug"
   echo "  cmake release"
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
 
mkdir -p ecbuild/$1
cd ecbuild/$1
      
cmake_build_type=
if [[ $1 = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

cmake ../.. -DCMAKE_BUILD_TYPE=$cmake_build_type \
            -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake  \
            -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$(cat ../../VERSION.cmake | awk '{print $3}'|sed 's/["]//g')

