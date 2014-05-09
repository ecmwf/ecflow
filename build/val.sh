#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This script will run valgrind on all the automated test
# Assume WK is defined

cd $WK

# if argument is release test the release version else stick with debug
mode=debug
if test "$1" = release
then
   mode=release
fi

compiler=gcc-$(gcc -dumpversion)

echo "valgrind: variant=$mode compiler=$compiler"

#
# Use valgrind which is newer than standard installation
#
# Note: To valgrind the server, start it separatelyon a different shell and define ECF_NODE=localhost
# export ECF_NODE=localhost
# valgrind Server/bin/$compiler/$mode/ecflow_server --interval=3
# Then restart this shell

valgrind --num-callers=24 --leak-check=full --show-reachable=yes ACore/bin/$compiler/$mode/u_acore
valgrind --num-callers=24 --leak-check=full --show-reachable=yes ANattr/bin/$compiler/$mode/u_anattr
valgrind --num-callers=24 --leak-check=full --show-reachable=yes ANode/bin/$compiler/$mode/u_anode
valgrind --num-callers=24 --leak-check=full --show-reachable=yes AParser/bin/$compiler/$mode/u_aparser
valgrind --num-callers=24 --leak-check=full --show-reachable=yes Base/bin/$compiler/$mode/u_base
valgrind --num-callers=24 --leak-check=full --show-reachable=yes Client/bin/$compiler/$mode/s_client
valgrind --num-callers=24 --leak-check=full --show-reachable=yes Server/bin/$compiler/$mode/u_server
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --num-callers=30 Test/bin/$compiler/$mode/s_test
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --num-callers=30 Test/bin/$compiler/$mode/s_test_zombies
valgrind --num-callers=24 --leak-check=full --show-reachable=yes CSim/bin/$compiler/$mode/c_csim
