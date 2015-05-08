#!/bin/sh

# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ======================================================================
# Use for install of ecflow using cmake and ecbuild(bundled with ecflow)
# External requirements: cmake, boost
#                        export WK=<source root of ecflow> i.e 
#                        /var/tmp/fred/ecflow_4_0_7

# assumes ecbuild is parallel to the source
# Will by default create a build directory parallel to source directory, i.e.
#                        /var/tmp/fred/cmake_build_dir
#

# Requires: users to set INSTALL_PREFIX,  
# This needs to be set to path where you want to install
INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow


# =====================================================================
# At least one argument expected:
if [ "$#" -ne 1 ] ; then
   echo "configure.sh expects 1 argument i.e"
   echo "  configure.sh  <prefix-dir>"
   echo "  configure.sh  /usr/local/apps/ecflow"
   exit 1
fi
INSTALL_PREFIX=$1

# ===================================================================
# Version number
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

# ===================================================================
# Build directory, always start clean, to avoid cmake cacheing issues
cd ..
if [ -e cmake_build_dir ] ; then
	rm -rf cmake_build_dir
fi
mkdir -p cmake_build_dir/ecflow/release
cd cmake_build_dir/ecflow/release


# =======================================================================
# On IBM we need to customise
cmake_extra_options=""

test_path ()
{
    if `command -v command 1>/dev/null 2>/dev/null`; then
        command -v $1 1>/dev/null 2>/dev/null
    else
        hash $1 1>/dev/null 2>/dev/null
    fi
}

test_uname ()
{
    if test_path uname; then
        test `uname` = $*
    fi
}

if test_uname AIX ; then
	cmake_extra_options="-DENABLE_GUI=OFF -DCMAKE_CXX_COMPILER=xlC_r"
fi

# ===================================================================
# Configuration

# BOOST_ROOT:  
#  By default it looks for environment variable BOOST_ROOT, if not it can specified on the command line. i.e
#  -DBOOST_ROOT=/var/tmp/ma0/boost/boost_1_53_0

# Python:
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
#    local : this will install to $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory

cmake $WK \
      -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX/$release.$major.$minor \
      -DCMAKE_PYTHON_INSTALL_TYPE=local  ${cmake_extra_options}
      #-DCMAKE_PYTHON_INSTALL_PREFIX=$INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow
