#!/bin/sh

## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This script will run valgrind on all the automated test
# Assume WK is defined

#===========================================================================
# load the latest valgrind
module load valgrind/3.11.0
#===========================================================================

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
# Note: To valgrind the server, start it separately on a different shell and define ECF_HOST=localhost
# export ECF_HOST=localhost
# valgrind Server/bin/$compiler/$mode/ecflow_server --interval=3
# Then restart this shell
   

# Use the valgrind option --track-origins=yes to have it track the origin of uninitialized values. 
# This will make it slower and take more memory, but can be very helpful if you need to track down 
# the origin of an uninitialized value.

valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes ACore/bin/$compiler/$mode/u_acore
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes ANattr/bin/$compiler/$mode/u_anattr
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes ANode/bin/$compiler/$mode/u_anode
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes AParser/bin/$compiler/$mode/u_aparser
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes Base/bin/$compiler/$mode/u_base
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes Client/bin/$compiler/$mode/s_client
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes Server/bin/$compiler/$mode/u_server
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --num-callers=30 --error-exitcode=1 --partial-loads-ok=yes Test/bin/$compiler/$mode/s_test
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --num-callers=30 --error-exitcode=1 --partial-loads-ok=yes Test/bin/$compiler/$mode/s_test_zombies
valgrind --num-callers=24 --leak-check=full --show-reachable=yes --error-exitcode=1 --partial-loads-ok=yes CSim/bin/$compiler/$mode/c_csim
