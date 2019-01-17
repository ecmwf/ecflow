#!/bin/sh

## Copyright 2009-2019 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

cd $WK

# if argument is release test the release version else stick with debug
mode=debug
if test "$1" = release
then
   mode=release
fi


compiler=gcc-$(gcc -dumpversion)
echo "valgrind: variant=$mode compiler=$compiler"

valgrind --tool=massif ACore/bin/$compiler/$mode/u_acore
valgrind --tool=massif ANattr/bin/$compiler/$mode/u_anattr
valgrind --tool=massif ANode/bin/$compiler/$mode/u_anode
valgrind --tool=massif ANode/parser/bin/$compiler/$mode/u_aparser
valgrind --tool=massif Base/bin/$compiler/$mode/u_base
valgrind --tool=massif Client/bin/$compiler/$mode/s_client
valgrind --tool=massif Server/bin/$compiler/$mode/u_server
valgrind --tool=massif Test/bin/$compiler/$mode/s_test
valgrind --tool=massif Simulator/bin/$compiler/$mode/c_csim
#valgrind --tool=massif ANode/parser/bin/$compiler/$mode/perf_aparser

