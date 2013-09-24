#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

## This will copy the boost tar file to the other platforms
## Assumes $WK defined

set -e # stop the shell on first error 
set -u # fail when using an undefined variable 

cd $WK
 
# =============================================================================================
# The *NEXT* version of boost
# =============================================================================================
BOOST_VERSION=1_51_0
BOOST_TAR_GZ_FILE=/var/tmp/ma0/boost/boost_$BOOST_VERSION.tar.gz

# ===============================================================================================
# IBM/ECGATE:  rs6000: GCC :  shared file system, home dir is accessible from ecgate
# ===============================================================================================
#HOST=ecgate
#ROOT_DIR=/emos_data/ecflow/rs6000/gcc
#BOOST_DIR=$ROOT_DIR/boost
#ecrcp $BOOST_TAR_GZ_FILE  $HOST:$BOOST_DIR/
 
# ===============================================================================================
# IBM/ECGATE: rs6000: XLC-v11.2  :  shared file system, home dir is accessible from ecgate
# ===============================================================================================
HOST=ecgate
ROOT_DIR=/emos_data/ecflow/rs6000/xlc
BOOST_DIR=$ROOT_DIR/boost
ecrcp $BOOST_TAR_GZ_FILE  $HOST:$BOOST_DIR/

# ===============================================================================================
# IBM-AIX: ibm_power6:   
# ===============================================================================================
HOST=c1b
ROOT_DIR=/s1a/emos_esuite/emos_data/sms
BOOST_DIR=$ROOT_DIR/boost
ecrcp $BOOST_TAR_GZ_FILE  $HOST:$BOOST_DIR

# ===============================================================================================
# IBM-AIX: ibm_power7:   
# ===============================================================================================
HOST=c2a
ROOT_DIR=/s2o1/emos_data/ecflow
BOOST_DIR=$ROOT_DIR/boost
ecrcp $BOOST_TAR_GZ_FILE  $HOST:$BOOST_DIR
  
# ==========================================================================================
# Linux-Cluster: 
# ==========================================================================================
HOST=lxb
ROOT_DIR=/vol/ecf/cluster

BOOST_DIR=$ROOT_DIR/boost
rcp $BOOST_TAR_GZ_FILE   lxa:$BOOST_DIR/

# ==========================================================================================
# Linux-opensuse113
# ==========================================================================================
HOST=opensuse113
ROOT_DIR=/vol/ecf/opensuse113
BOOST_DIR=$ROOT_DIR/boost
rcp $BOOST_TAR_GZ_FILE   $HOST:$BOOST_DIR/

# ==========================================================================================
# Linux-redhat   new ecgate
# ==========================================================================================
HOST=ecgb
ROOT_DIR=/vol/ecf/redhat
BOOST_DIR=$ROOT_DIR/boost
rcp $BOOST_TAR_GZ_FILE   $HOST:$BOOST_DIR/

# ==========================================================================================
# Linux-opensuse103
# ==========================================================================================
HOST=opensuse103
ROOT_DIR=/vol/ecf/opensuse103
BOOST_DIR=$ROOT_DIR/boost
rcp $BOOST_TAR_GZ_FILE   $HOST:$BOOST_DIR/

# =======================================================================================
# HP-UX, scratch is shared/accessible from linux
# =======================================================================================
HOST=itanium
ROOT_DIR=/scratch/ma/emos/ma0/hpia64
BOOST_DIR=$ROOT_DIR/boost
rcp $BOOST_TAR_GZ_FILE   $BOOST_DIR/
