#!/bin/ksh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

## OLD bjam based install, not used anymore. kept for reference
##
## This script will install ecflow dependent on the host
##
## Expect an optional argument [ debug | release | test ]
##  - debug   : install a debug version of ecflow
##  - release : install a release version of ecflow
##  - test    : test install
##
## ie install_ecflow.sh debug test
##    Will do a test install for debug build of ecflow
##
## Date Changed: 23/05/2011

set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ====================================================================
# Check user is logged in as emos
user="$(whoami)"
if [[ $user != "emos" ]] && [[ $user != "map" ]] ; then
   echo "Must be logged in as emos to install ecflow"
   exit 1
fi

# ====================================================================
# Maximum of 2 arguments expected:
#    o/ platform
#    o/ input parameter
if [ "$#" -gt 2 ] ; then
   echo "Install expects maximum of two arguments, i.e"
   echo "  test debug"
   echo "  test release"
   echo "  test"
   echo "  debug"
   echo "  release"
   exit 1
fi

mode_arg= 
test_arg=
while [[ "$#" != 0 ]] ; do   
   if [[ "$1" != debug && "$1" != release && "$1" != test ]] ; then
      echo "Install expected [ debug | release | test ] but found $1"
      exit 1
   fi
   if [[ "$1" = debug || "$1" = release ]] ; then
      mode_arg=$1
   else
      test_arg=-n
   fi
   # shift remove last argument
   shift
done

if [ ${#mode_arg} -eq 0 ] ; then
   echo "Install expects mode i.e. debug or release"
   exit 1
fi

install_arg=install-all

# ==============================================================================
# Expect $WK and $BOOST_ROOT to be specified
# ==============================================================================
if [ "${WK:-unset}" = "unset" ] ; then
   echo "Install expects environment variable WK  work space to be defined"
   exit 1
fi  
if [ "${BOOST_ROOT:-unset}" = "unset" ] ; then
   echo "Install expects environment variable BOOST_ROOT to be defined"
   exit 1
fi  
echo "WK         = $WK"
echo "BOOST_ROOT = $BOOST_ROOT"

# ==============================================================================
# In order to embedd boost_python path in the ecflow extension, we need
# ECFLOW_INSTALL_DIR to be set correctly, when building the extension
# Hacky work around since, <dll-path> does not work for a relink at install time.
# Set install directory for ecflow & embedding of boost python extension
# ===============================================================================
cd $WK

# Determine the release,major,minor numbers for this version 
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
ECFLOW_VERSION=$release.$major.$minor
   
export ECFLOW_INSTALL_DIR=${ECFLOW_INSTALL_DIR:-/usr/local/apps/ecflow/${release}.${major}.${minor}}

# =============================================================================
# Required for cray, since we allow multiple compilers
# =============================================================================
TOOLSET=
CXXFLAGS=

# ======================================================================
# We do NOT install on the LOCAL build machine, since that will not have
# the correct embedded paths with ecflow.so (i.e for boost python )
# ======================================================================

BOOST_VERSION=boost_1_53_0

if [[ "$user" = "map" ]] # when user emos is commented out
then
   ARCH=linux
   export BOOST_ROOT=${BOOST_ROOT:-/vol/ecf/opensuse113/boost/$BOOST_VERSION};  
   export WK=${WK:-/vol/ecf/opensuse113/ecflow}

elif [[ "$ARCH" = "Linux" ]] || [[ "$ARCH" = "linux" ]] 
then  
   # =====================================================================
   # LINUX
   # =====================================================================
   
   # lxop does not define OS_VERSION ?????, hence default to empty string
   : ${OS_VERSION:=""}
   
   if [[ "$OS_VERSION" = opensuse113 ]] ; then
   
      export BOOST_ROOT=/vol/ecf/opensuse113/boost/$BOOST_VERSION; 
      export WK=/vol/ecf/opensuse113/ecflow

   elif [[ "$OS_VERSION" = opensuse131 ]] ; then
   
      export BOOST_ROOT=/vol/ecf/opensuse131/boost/$BOOST_VERSION; 
      export WK=/vol/ecf/opensuse131/ecflow
      
   elif [[ "$OS_VERSION" = opensuse103 ]] ; then 
   
      export BOOST_ROOT=/vol/ecf/opensuse103/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/opensuse103/ecflow
      
   elif [[ "$OS_VERSION" = rhel6 ]] ; then 

      export BOOST_ROOT=/vol/ecf/redhat/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/redhat/ecflow
   
   elif [[ "$OS_VERSION" = sles11 ]] ; then 
   
      # lxab this is still opensuse113
      export BOOST_ROOT=/vol/ecf/cluster/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/cluster/ecflow
      case "$HOST" in
         lxc*)
	           export BOOST_ROOT=/vol/ecf/lxc/boost/$BOOST_VERSION;  
	           export WK=/vol/ecf/lxc/ecflow
         ;;
	  esac
	  
   elif [[ "$OS_VERSION" = "" ]] ; then 
   
      # lxop does not define OS_VERSION ?????
      export BOOST_ROOT=/gpfs/lxop/build/builds/boost/$BOOST_VERSION;  
      export WK=/gpfs/lxop/build/builds/ecflow
   fi
  
elif [[ "$ARCH" = cray ]] ; then 

   TOOLSET=toolset=gcc
   CXXFLAGS=cxxflags=-fPIC
   if [[ "$PE_ENV" = CRAY ]] ; then
      echo "The PE_ENV=CRAY, For ecflow we only support install with PE_ENV=GNU"
      exit 1
   fi
   if [[ "$PE_ENV" = INTEL ]] ; then
      echo "The PE_ENV=INTEL, For ecflow we only support install with PE_ENV=GNU"
      exit 1
   fi
   
   export WK=/perm/ma/ma0/workspace/$PE_ENV/ecflow
   export BOOST_ROOT=/perm/ma/ma0/boost/$BOOST_VERSION
fi

# =======================================================================================
# Python
# ========================================================================================
export ECFLOW_PYTHON_INSTALL_DIR=$ECFLOW_INSTALL_DIR/lib/python2.7/site-packages/ecflow 


# ============================================================================
# INSTALL
# ============================================================================
$BOOST_ROOT/bjam $TOOLSET $CXXFLAGS -d2 variant=$mode_arg $test_arg $install_arg
   

#==========================================================================
echo "...make sure executables have execute permissions for group and others"
# ============================================================================
if [[ "$test_arg" = "" ]] ; then
   cd $ECFLOW_INSTALL_DIR/bin
   chmod 755 *
   
   cd $ECFLOW_PYTHON_INSTALL_DIR
   chmod 755 *
   
   #==========================================================================
   echo "...sanity test, make sure exe's exist in the bin directory"
   # ============================================================================
   if [[ ! -r ecflow_client ]] ; then
      echo "ecflow_client not installed !!!!!"
   fi
   if [[ ! -r ecflow_server ]] ; then
      echo "ecflow_server not installed !!!!!"
   fi
   if [[ ! -r ecflowview ]] ; then
      echo "ecflowview not installed !!!!!"
   fi
fi

# ============================================================================ 
# Copy over release from ccb -> cca, or cca ->ccb, depending on the machine 
# *Make* sure destination has a trailing '/.' otherwise you can end up renaming.
# ============================================================================
if [[ "$ARCH" = cray ]] ; then 

   if [[ "$test_arg" = "" ]] ; then
      cd /usr/local/apps/ecflow
      
      if [[ "$EC_CLUSTER" = ccb ]] ; then
      
      	 scp -r $ECFLOW_VERSION emos@cca:/usr/local/apps/ecflow/.
      	 
      elif [[ "$EC_CLUSTER" = cca ]] ; then
      
        scp -r $ECFLOW_VERSION emos@ccb:/usr/local/apps/ecflow/.
        
      fi
   fi
fi  


# ============================================================================ 
# Copy over release from ecgb(redhat) -> sappa and sappb
# *Make* sure destination has a trailing '/.' otherwise you can end up renaming.
# ============================================================================
if [[ "$ARCH" = "Linux" ]] || [[ "$ARCH" = "linux" ]] 
then  
   if [ "$OS_VERSION" = rhel6 ] ; then
   
      if [[ "$test_arg" = "" ]] ; then
         # sappa/sappb(rhel63) are same as ecgb/redhat(rhel6)
         cd /usr/local/apps/ecflow
         scp -r $ECFLOW_VERSION emos@sappa:/usr/local/apps/ecflow/.
         scp -r $ECFLOW_VERSION emos@sappb:/usr/local/apps/ecflow/.
      fi
   fi
fi