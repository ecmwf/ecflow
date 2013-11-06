#!/bin/ksh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

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
# In order to embedd boost_python path in the ecflow extension, we need
# ECFLOW_INSTALL_DIR to be set correctly, when building the extension
# Hacky work around since, <dll-path> does not work for a relink at install time.
# Set install directory for ecflow & embedding of boost python extension
# ===============================================================================
cd $WK

# Determine the release,major,minor numbers for this version 
release=$(grep "Version::release_" ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g')
major=$(grep   "Version::major_"   ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g')
minor=$(grep   "Version::minor_"   ACore/src/Version.cpp  | cut -d= -s -f2 | sed 's/;//g' | sed 's/ //g' | sed 's/"//g' )
ECFLOW_VERSION=$release.$major.$minor
   
export ECFLOW_INSTALL_DIR=${ECFLOW_INSTALL_DIR:-/usr/local/apps/ecflow/$release.$major.$minor}


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
   if [ "$OS_VERSION" = opensuse113 ] ; then
   
      export BOOST_ROOT=/vol/ecf/opensuse113/boost/$BOOST_VERSION; 
      export WK=/vol/ecf/opensuse113/ecflow
      
   elif [ "$OS_VERSION" = opensuse103 ] ; then 
   
      export BOOST_ROOT=/vol/ecf/opensuse103/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/opensuse103/ecflow
      
   elif [ "$OS_VERSION" = rhel6 ] ; then 

      export BOOST_ROOT=/vol/ecf/redhat/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/redhat/ecflow
   
   elif [ "$OS_VERSION" = sles11 ] ; then 
   
      # lxab this is still opensuse113
      export BOOST_ROOT=/vol/ecf/cluster/boost/$BOOST_VERSION;  
      export WK=/vol/ecf/cluster/ecflow
   fi
  
elif [[ "$ARCH" = cray ]] ; then 

   if [[ "$PE_ENV" = CRAY ]] ; then
      # export COMPILER_VERSION=$(echo $CRAY_CC_VERSION | sed 's/\.//' | cut -c1-2)
      echo "The PE_ENV=CRAY, For ecflow we only support install with PE_ENV=GNU"
      exit 1
   fi
   if [[ "$PE_ENV" = INTEL ]] ; then
      # export COMPILER_VERSION=$(icc -dumpversion | sed 's/\.//' | cut -c1-3)
      echo "The PE_ENV=INTEL, For ecflow we only support install with PE_ENV=GNU"
      exit 1
   fi
   if [[ "$PE_ENV" = GNU ]] ; then
      export COMPILER_VERSION=$(gcc -dumpversion | sed 's/\.//' | cut -c1-2)
   fi
   #export ECFLOW_INSTALL_DIR=/usr/local/apps/ecflow/$release.$major.$minor/$PE_ENV/$COMPILER_VERSION
   
   export WK=/perm/ma/ma0/workspace/$PE_ENV/ecflow
   export BOOST_ROOT=/perm/ma/ma0/boost/$BOOST_VERSION
   echo "WK = $WK"
   echo "BOOST_ROOT = $BOOST_ROOT"
   echo "ECFLOW_INSTALL_DIR = $ECFLOW_INSTALL_DIR"
   echo "COMPILER_VERSION = $COMPILER_VERSION"

elif [[ "$ARCH" = hpia64 ]] ; then 

   # ======================================================================
   # HPUX:   We don't install ecflowview on HPUX, no x-windows
   # ======================================================================
   NEW_SCRATCH=/scratch/ma/emos/ma0
   export BOOST_ROOT=$NEW_SCRATCH/$ARCH/boost/$BOOST_VERSION;  
   export WK=$NEW_SCRATCH/$ARCH/ecflow   
      
   install_arg=install 
   
elif [[ "$ARCH" = ibm_power6 ]] ; then 

   # ======================================================================
   # AIX:   We don't install ecflowview on AIX, no x-windows
   # ======================================================================
   export BOOST_ROOT=/s1a/emos_esuite/emos_data/sms/boost/$BOOST_VERSION;  
   export WK=/s1a/emos_esuite/emos_data/sms/ecflow 
   
   install_arg=install 

elif [[ "$ARCH" = ibm_power7 ]] ; then 

   # ======================================================================
   # AIX:   We don't install ecflowview on AIX, no x-windows
   # ======================================================================
   export BOOST_ROOT=/s2o1/emos_data/ecflow/boost/$BOOST_VERSION;  
   export WK=/s2o1/emos_data/ecflow/ecflow 
   
   install_arg=install 
   
elif [[ "$ARCH" = rs6000 ]] ; then 

   export BOOST_ROOT=/emos_data/ecflow/rs6000/xlc/boost/$BOOST_VERSION;  
   export WK=/emos_data/ecflow/rs6000/xlc/ecflow    
fi

# =======================================================================================
# Python
# ========================================================================================
export ECFLOW_PYTHON_INSTALL_DIR=$ECFLOW_INSTALL_DIR/lib/python2.7/site-packages/ecflow 


# ============================================================================
# INSTALL
# ============================================================================
$BOOST_ROOT/bjam -d2 variant=$mode_arg $test_arg $install_arg
   
# ===========================================================================
# install system files for ecmwf configuration: servers list + menu
# ===========================================================================
if [[ "$test_arg" = "" ]] ; then
   DEST=$ECFLOW_INSTALL_DIR/lib
   cp -f $WK/build/servers $WK/view/src/ecflowview.menu $DEST/.
   chmod 644 $DEST/servers $DEST/ecflowview.menu
fi
   
# ============================================================================ 
# Copy over release from c2a -> c2b
#
# *Make* sure destination has a trailing '/.' otherwise you can end up renaming.
# =============================================================================
if [[ "$ARCH" = ibm_power7 ]] ; then 

   if [[ "$test_arg" = "" ]] ; then
      cd /usr/local/apps/ecflow
      ecrcp -avr $ECFLOW_VERSION emos@c2b:/usr/local/apps/ecflow/.
   fi
fi  

if [[ "$test_arg" = "" ]] ; then
   #==========================================================================
   echo "...make sure executables have execute permissions for group and others"
   # ============================================================================
   cd $ECFLOW_INSTALL_DIR/bin
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
