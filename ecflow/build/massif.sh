#!/bin/sh

## Copyright 2009-2012 ECMWF. 
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

valgrind --tool=massif ACore/bin/$compiler/$mode/coretest
valgrind --tool=massif ANattr/bin/$compiler/$mode/nodeattrtest
valgrind --tool=massif ANode/bin/$compiler/$mode/nodetest
valgrind --tool=massif AParser/bin/$compiler/$mode/tparser
valgrind --tool=massif Base/bin/$compiler/$mode/basetest
valgrind --tool=massif Client/bin/$compiler/$mode/tclient
valgrind --tool=massif Server/bin/$compiler/$mode/tserver
valgrind --tool=massif Test/bin/$compiler/$mode/test
valgrind --tool=massif Simulator/bin/$compiler/$mode/testsimulator
#valgrind --tool=massif AParser/bin/$compiler/$mode/tsingle

