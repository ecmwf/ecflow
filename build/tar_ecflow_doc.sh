#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# tar up ecflow documentation.
#  - consisting of html based online tutorial and python API
#  - user manual in Word and PDF formats
# Creates a file of the form ecflow_2_0_19_doc.tar.gz in directory $SCRATCH
# Assumes $WK(root workspace) defined
# Assumes $SCRATCH is defined
# Assumes sphinx available to build online documentation
#

set -e # stop the shell on first error 
set -u # fail when using an undefined variable 

# ======================================================================
# Determine ecflow build directory name: see ACore/doc/extracting_version_number.ddoc
# ========================================================================
cd $WK

release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

ECFLOW_WS_DIR=ecflow_${release}_${major}_${minor}_doc


# ===================================================================================
# generate online tutorial so the we don't need to add dependency 
# on sphinx build on other platforms
# ===================================================================================
cd $WK/Doc/online
make clean
make html


# ================================================================================
# remove old doc tar files
# ================================================================================
ECFLOWTAR=$ECFLOW_WS_DIR.tar
rm -rf $SCRATCH/$ECFLOWTAR.gz
rm -rf $SCRATCH/$ECFLOWTAR 


# ================================================================================
# Create a tar of the online and user manual documentation
# Notice: we don't delete Doc/online/_build as this contains the index.html
# ================================================================================
# Exclusions from tar file: 
# o/ func_spec 
# o/ .metadata, .cproject, .project, .settings, csettings are eclipse dir
# o/ newsletter
# o/ presentation
# o/ Tac
# o/ misc
#
cd $WK
tar --exclude=func_spec \
    --exclude=newsletter \
    --exclude=presentations \
    --exclude=seminar \
    --exclude=ecbuild \
    --exclude=tac \
    --exclude=misc \
    --exclude=.metadata --exclude=.cproject --exclude=.project --exclude=.settings --exclude=.csettings \
    --exclude=Thumbs.db \
    -cf $ECFLOWTAR Doc/

ls -lh $ECFLOWTAR
gzip $ECFLOWTAR
mv $ECFLOWTAR.gz $SCRATCH/

