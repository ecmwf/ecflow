#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# Sync up before export/build
 
set -e # stop the shell on first error 
set -u # fail when using an undefined variable 


# P4 login
cd $HOME
rm -rf .p4tickets
p4login

 
# setup for perforce server
cd $WK
export P4CLIENT=clientWS
export P4PORT=p4od:14001

p4 sync
