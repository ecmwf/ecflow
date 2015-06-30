#!/bin/ksh

# ==================================================================
# Setup environment on CRAY, allow switch between cray,intel and gnu
# ==================================================================

#set -e # stop the shell on first error
#set -u # fail when using an undefined variable
#set -x # echo script lines as they are executed

if [[ "$#" != 1 ]] ; then
   echo "Expect one of [ gnu, intel, cray ] "
   exit 1
else 
  if [[ "$1" != cray && "$1" != intel && "$1" != gnu ]] ; then
     echo "Expect one of [ gnu, intel, cray ] "
     exit 1
  fi
fi

echo "STARTING   PE_ENV=$PE_ENV"

# access the module functionality
. /opt/modules/default/etc/modules.sh
   
# setup boost
export BOOST_ROOT=/perm/ma/ma0/boost/boost_1_53_0
cp $WK/build_scripts/site_config/site-config-cray.jam $BOOST_ROOT/tools/build/v2/site-config.jam
   
if [[ "$1" = cray ]] ; then
   if [[ "$PE_ENV" = INTEL ]] ; then
      module swap PrgEnv-intel PrgEnv-cray
   fi
   if [[ "$PE_ENV" = GNU ]] ; then
      module swap PrgEnv-gnu PrgEnv-cray
   fi
   
   module unload cce
   module load cce/8.3.0.186
   export COMPILER_VERSION=$(echo $CRAY_CC_VERSION | sed 's/\.//' | cut -c1-2)
   export WK=/perm/ma/ma0/workspace/$PE_ENV/ecflow
   export BOOST_ROOT=/perm/ma/ma0/boost/boost_1_55_0
   cp $WK/build_scripts/site_config/site-config-cray.jam $BOOST_ROOT/tools/build/v2/site-config.jam
   alias bjam='$BOOST_ROOT/bjam cxxflags=-hPIC toolset=cray'
   
   # module cray-libsci interferes with ecflow linking, hence disable
   module unload cray-libsci
fi


if [[ "$1" = intel ]] ; then
   if [[ "$PE_ENV" = CRAY ]] ; then
      module swap PrgEnv-cray PrgEnv-intel
   fi
   if [[ "$PE_ENV" = GNU ]] ; then
      module swap PrgEnv-gnu PrgEnv-intel
   fi
   
   # for compiler version icc -dumpversion
   export COMPILER_VERSION=$(icc -dumpversion | sed 's/\.//' | cut -c1-3)
   export WK=/perm/ma/ma0/workspace/$PE_ENV/ecflow
   alias bjam='$BOOST_ROOT/bjam cxxflags=-fPIC toolset=intel'
fi

if [[ "$1" = gnu ]] ; then
   if [[ "$PE_ENV" = CRAY ]] ; then
      module swap PrgEnv-cray PrgEnv-gnu
   fi
   if [[ "$PE_ENV" = INTEL ]] ; then
      module swap PrgEnv-intel PrgEnv-gnu
   fi
   
   # for compiler version gcc -dumpversion
   #module load gcc/4.6.3
   
   export COMPILER_VERSION=$(gcc -dumpversion | sed 's/\.//' | cut -c1-2)
   export WK=/perm/ma/ma0/workspace/$PE_ENV/ecflow
   alias bjam='$BOOST_ROOT/bjam cxxflags=-fPIC toolset=gcc'     
fi


# =================================================================================================
# Determine the release,major,minor numbers for this version 
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
ECFLOW_VERSION=$release.$major.$minor
  
export ECFLOW_INSTALL_DIR=/usr/local/apps/ecflow/$release.$major.$minor
export ECFLOW_PYTHON_INSTALL_DIR=$ECFLOW_INSTALL_DIR/lib/python/2.7/site-packages/ecflow 


echo "AFTER SWAP PE_ENV=$PE_ENV"
echo "WK=$WK"
echo "BOOST_ROOT=$BOOST_ROOT"
echo "COMPILER_VERSION=$COMPILER_VERSION"
echo "ECFLOW_INSTALL_DIR=$ECFLOW_INSTALL_DIR"  
echo "ECFLOW_PYTHON_INSTALL_DIR=$ECFLOW_PYTHON_INSTALL_DIR"  

