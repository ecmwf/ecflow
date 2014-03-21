#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# tar up all writable files from a given set of extensions
# Copies file over to HPUX, AIX and linux cluster
# Assumes $WK defined

set -e # stop the shell on first error 
set -u # fail when using an undefined variable 

# remove incremental old tar files & left over old links
cd $WK
cd ..
rm -rf incr.tar
rm -rf incr.tar.gz

#
# Test/data/ECF_HOME is used by Test to recursively generate directory structure
# and populate with *DEF* file, and job output, etc, hence exclude this directory
#
rm -rf $WK/Test/data/ECF_HOME_debug*
rm -rf $WK/Test/data/ECF_HOME_release*
rm -rf $WK/AParser/test/data/single_defs/mega.def_log
rm -rf $WK/Pyext/test.def

# Remove any lock file create by tests which used EcfPortLock.hpp
rm -rf $WK/*.lock


# ==============================================================================================
# TAR
# temporarily create a symbolic link, so that tar file matches directory ecflow.
# ==============================================================================================
cd $WK
cd ..
ln -s ecflow ecflow

# Recursively find in directory MyProject all writable:
#     cpp, c,hpp,sh, defs files, .ecf, jam and py files and tar/gzip them up.
find ecflow/.  \
     \( -name *.cpp    \
        -o -name *.c   \
        -o -name *.cc  \
        -o -name *.hpp \
        -o -name *.h   \
        -o -name *.y   \
        -o -name Makefile   \
        -o -name *.jam \
        -o -name *.py  \
        -o -name *.sh  \
        -o -name *.def \
        -o -name *.ecf \
     \) -perm -u=w -print | \
        xargs tar \
         --exclude=.metadata \
         --exclude=_build --exclude=func_spec --exclude=misc --exclude=presentations --exclude=seminar --exclude=tac --exclude=user-manual \
         --exclude=.pydevproject \
         --exclude=ecbuild \
         --exclude=gcc4.2.1 \
         --exclude=bin \
         --exclude=include \
         --exclude=SCRATCH \
         --exclude=CUSTOMER \
         --exclude=RemoteSystemsTempFiles \
         --exclude=test_bench \
         --exclude=test_view \
         --exclude=nightly \
         --exclude=Xcdp \
         --exclude=xcdp \
         -cf incr.tar
        
gzip incr.tar
 
# Remove the link
rm ecflow  

BOOST_VERSION=boost_1_47_0

# ===============================================================================
# copy to c2a/power7 ***** hard coded boost root *****
# ===============================================================================
IBM=c2a
ecrcp $WK/build/site_config/site-config-AIX.jam $IBM:/s2o1/emos_data/ecflow/boost/$BOOST_VERSION/tools/build/v2/site-config.jam
ecrcp $HOME/.user_kshrc                         $IBM: 
ecrcp incr.tar.gz                               $IBM:/s2o1/emos_data/ecflow/
ecrcp $WK/build/untar_incr_ecflow.sh            $IBM:/s2o1/emos_data/ecflow/
rsh $IBM "cd /s2o1/emos_data/ecflow/; chmod 755 untar_incr_ecflow.sh; sh -x untar_incr_ecflow.sh"

# ===============================================================================
# Linux cluster:  lxa, is very slow, hence switched to lxb, gcc-4.3
# ===============================================================================
rcp incr.tar.gz                       lxb:/vol/ecf/cluster
rsh lxb "cd /vol/ecf/cluster; rm -rf untar_incr_ecflow.sh"
rcp $WK/build/untar_incr_ecflow.sh    lxb:/vol/ecf/cluster/
rsh lxb "cd /vol/ecf/cluster/; chmod 755 untar_incr_ecflow.sh; sh -x untar_incr_ecflow.sh"

# ===============================================================================
# Linux opensuse103:    
# ===============================================================================
rcp incr.tar.gz                       opensuse103:/vol/ecf/opensuse103
rsh opensuse103 "cd /vol/ecf/opensuse103/; rm -rf untar_incr_ecflow.sh"
rcp $WK/build/untar_incr_ecflow.sh    opensuse103:/vol/ecf/opensuse103/
rsh opensuse103 "cd /vol/ecf/opensuse103/; chmod 755 untar_incr_ecflow.sh; sh -x untar_incr_ecflow.sh"

# ===============================================================================
# Linux opensuse113:    
# ===============================================================================
rcp incr.tar.gz                       opensuse113:/vol/ecf/opensuse113
rsh opensuse113 "cd /vol/ecf/opensuse113/; rm -rf untar_incr_ecflow.sh"
rcp $WK/build/untar_incr_ecflow.sh    opensuse113:/vol/ecf/opensuse113/
rsh opensuse113 "cd /vol/ecf/opensuse113/; chmod 755 untar_incr_ecflow.sh; sh -x untar_incr_ecflow.sh"

# ===============================================================================
# Linux redhat:    
# ===============================================================================
scp incr.tar.gz                       ecgb:/vol/ecf/redhat
ssh ecgb "cd /vol/ecf/redhat/; rm -rf untar_incr_ecflow.sh"
scp $WK/build/untar_incr_ecflow.sh    ecgb:/vol/ecf/redhat/
ssh ecgb "cd /vol/ecf/redhat/; chmod 755 untar_incr_ecflow.sh; sh -x untar_incr_ecflow.sh"

# =================================================================================
# HPUX: Note $SCRATCH is a very slow system
# =================================================================================
# The persistent file system.
ARCH=hpia64
 
# if the incr tar file exist remove it
NEW_SCRATCH=/scratch/ma/emos/ma0
rm -rf $NEW_SCRATCH/$ARCH/incr.tar
rm -rf $NEW_SCRATCH/$ARCH/incr.tar.gz
 
cd $WK
cd ..  
cp incr.tar.gz                    $NEW_SCRATCH/$ARCH
cp $WK/build/untar_incr_ecflow.sh $NEW_SCRATCH/$ARCH

cd $NEW_SCRATCH/$ARCH
chmod 755 untar_incr_ecflow.sh
sh -x untar_incr_ecflow.sh 
