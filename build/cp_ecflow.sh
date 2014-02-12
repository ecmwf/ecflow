#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

## This will tar up ecflow for copying to platforms AIX,HPUX,linux cluster
## Assumes $WK defined

set -e # stop the shell on first error 
set -u # fail when using an undefined variable 

# ======================================================================
# Determine version number: see ACore/doc/extracting_version_number.ddoc
# ========================================================================
cd $WK
ecflow_version=$(cat $WK/VERSION.cmake | awk '{print $3}'|sed 's/["]//g')
release=$(echo $ecflow_version | cut -d. -f1)
major=$(echo $ecflow_version   | cut -d. -f2)
minor=$(echo $ecflow_version   | cut -d. -f3)
ECFLOW=ecflow_${release}_${major}_${minor}

# ============================================================================================
# Copy test.sh & rmbin,sh to $HOME/bin/
# ============================================================================================
cp build/test.sh  $HOME/bin/test.sh
cp build/rmbin.sh $HOME/bin/rmbin.sh

# ============================================================================================
# Create a file ecflow.tar.gz/ This is created **one directory*** up
# ============================================================================================
sh -x build/tar_ecflow.sh

# cd to where tar file directory resides
cd ..
ECFLOWTAR=$ECFLOW.tar
 
# =============================================================================================
# The current tested boost version.
# Note: build_boost.sh needs to be moved into the boost directory before it is run
#                      It specifies the minimum boost libs required, and library format
#                      expected for linking in release and debug modes.
#  build_boost.sh Will also copy the platform specific site-config files, required for building ecFlow
BOOST_VERSION=1_47_0
BOOST_TAR_GZ_FILE=/var/tmp/ma0/boost/boost_$BOOST_VERSION.tar.gz


# =================================================================================================
# For all installation assumes: 
# o python & python-devel packages for building ext modules has been installed.


# ===============================================================================================
# IBM-AIX: ibm_power6:   
# ===============================================================================================
HOST=c1b
ROOT_DIR=/s1a/emos_esuite/emos_data/sms
ecrcp $WK/build/test.sh     $HOST:bin/
ecrcp $WK/build/rmbin.sh    $HOST:bin/
ecrcp $HOME/.user_kshrc     $HOST: 

rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"  
ecrcp $ECFLOWTAR.gz         $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"
 
BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-AIX.jam   $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam
  
# ===============================================================================================
# IBM-AIX: ibm_power7:   
# ===============================================================================================
HOST=c2a
ROOT_DIR=/s2o1/emos_data/ecflow
ecrcp $WK/build/test.sh     $HOST:bin/
ecrcp $WK/build/rmbin.sh    $HOST:bin/
ecrcp $HOME/.user_kshrc     $HOST: 

rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"  
ecrcp $ECFLOWTAR.gz         $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"
 
BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-AIX.jam   $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam
  
  
# ==========================================================================================
# Linux-Cluster: 
# ==========================================================================================
HOST=lxb
ROOT_DIR=/vol/ecf/cluster
rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"
rcp $ECFLOWTAR.gz  $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"

BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-Linux64.jam $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam

# ==========================================================================================
# Linux-opensuse113
# ==========================================================================================
HOST=opensuse113
ROOT_DIR=/vol/ecf/opensuse113
rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"
rcp $ECFLOWTAR.gz  $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"

BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-Linux64.jam  $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam

# ==========================================================================================
# Linux-redhat
# ==========================================================================================
HOST=ecgb
ROOT_DIR=/vol/ecf/redhat
rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"
rcp $ECFLOWTAR.gz  $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"

BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-Linux64.jam  $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam

# ==========================================================================================
# Linux-opensuse103, 
# ==========================================================================================
HOST=opensuse103
ROOT_DIR=/vol/ecf/opensuse103
rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"
rcp $ECFLOWTAR.gz  $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"

BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-Linux.jam  $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam

# =======================================================================================
# HP-UX, scratch is shared/accessible from linux
# =======================================================================================
HOST=itanium
ROOT_DIR=/scratch/ma/emos/ma0/hpia64
rsh $HOST "cd $ROOT_DIR; rm -rf ecflow_*"
rcp $ECFLOWTAR.gz  $HOST:$ROOT_DIR/
rsh $HOST "cd $ROOT_DIR; gunzip $ECFLOWTAR.gz; tar -xf $ECFLOWTAR"

BOOST_DIR=$ROOT_DIR/boost
rcp $WK/build/site_config/site-config-HPUX.jam  $HOST:$BOOST_DIR/boost_$BOOST_VERSION/tools/build/v2/site-config.jam
