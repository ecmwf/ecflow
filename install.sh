#!/bin/sh
# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -o pipefail # fail if last(rightmost) command exits with a non-zero status

# ====================================================================
show_error_and_exit() {
   echo "install.sh "
   echo " install.sh  [prefix]"
   echo "  "
   echo "   prefix         - prefix directory for install"
   exit 1
}

prefix_arg=/var/tmp/$USER/install/cmake/ecflow
if  [[ "$#" = 1 ]] ; then
   prefix_arg=$1
fi

set -x # echo script lines as they are executed

#==========================================================================================
mkdir -p build
cd build

#=======================================================================================
# -DCMAKE_PYTHON_INSTALL_TYPE
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ] # if not specified defaults to local.
# -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#   *AND* for testing python install to local directory.
#  Using make -j8 may fail when using   '-DCMAKE_PYTHON_INSTALL_TYPE=setup' use:
#    > make # OR
#    > make -j8 -k
#
# Boost:  
#  By default it looks for environment variable BOOST_ROOT, if not it can specified on the command line. i.e
#  -DBOOST_ROOT=/var/tmp/ma0/boost/boost_1_53_0
#
# -DCMAKE_BUILD_TYPE: The default build is RelWithDebInfo for ecbuild, this has been
#                     overridden in the top level CMakeLists.txt to Release
#                     It can still be overridden specifying on the command line.
#                     Be sure to remove cache first.

cmake .. -DCMAKE_INSTALL_PREFIX=$prefix_arg  \
         # -DCMAKE_BUILD_TYPE=Release 
         # -DENABLE_GUI=OFF       \
         # -DENABLE_UI=OFF        \
         # -DENABLE_PYTHON=OFF    \
         # -DENABLE_ALL_TESTS=ON  \
         # -DCMAKE_PYTHON_INSTALL_TYPE=local \
         # -DCMAKE_PREFIX_PATH="/usr/local/apps/qt/5.5.0/5.5/gcc_64/" \
         # -DCMAKE_CXX_FLAGS="-Wall -Wno-unused-local-typedefs" \
         # -DCMAKE_CXX_COMPILER=xlC_r"
         # -DBOOST_ROOT=/var/tmp/$USER/boost/boost_1_53_0
         
make -j8 -k
make install
             