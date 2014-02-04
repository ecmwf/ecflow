#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

## This will untar the boost build on the supported platforms
## Assumes $WK defined

set -e # stop the shell on first error 
set -u # fail when using an undefined variable 

cd $WK
 
# =============================================================================================
# The *NEXT* boost version to try
BOOST_VERSION=1_48_0
 
# ===============================================================================================
# IBM-AIX: ibm_power6:   
# ===============================================================================================
HOST=c1b
ROOT_DIR=/s1a/emos_esuite/emos_data/sms

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"

# ===============================================================================================
# IBM-AIX: ibm_power7:   
# ===============================================================================================
HOST=c2a
ROOT_DIR=/s2o1/emos_data/ecflow

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"
   
  
# ==========================================================================================
# Linux-Cluster: gcc4.3
# ==========================================================================================
HOST=lxb
ROOT_DIR=/vol/ecf/cluster

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"
 

# ==========================================================================================
# Linux-opensuse113, gcc-4.5
# ==========================================================================================
HOST=opensuse113
ROOT_DIR=/vol/ecf/opensuse113

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"

# ==========================================================================================
# Linux-redhat, gcc-4.4
# ==========================================================================================
HOST=ecgb
ROOT_DIR=/vol/ecf/redhat

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"
 
# ==========================================================================================
# Linux-opensuse103, gcc 4.2.1
# ==========================================================================================
HOST=opensuse103
ROOT_DIR=/vol/ecf/opensuse103

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"
 

# =======================================================================================
# HP-UX, scratch is shared/accessible from linux
# =======================================================================================
HOST=itanium
ROOT_DIR=/scratch/ma/emos/ma0/hpia64

BOOST_DIR=$ROOT_DIR/boost
rsh $HOST "cd $BOOST_DIR; gunzip boost_$BOOST_VERSION.tar.gz; tar -xf boost_$BOOST_VERSION.tar"
