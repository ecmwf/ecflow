#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# will run gprof

echo "building profile versions";
cd $WK
bjam -j2 -q variant=profile

 
compiler=gcc-$(gcc -dumpversion)
echo "gprof: variant=profile compiler=$compiler"

#
cd ACore
bin/$compiler/profile/coretest
gprof bin/$compiler/profile/coretest   gmon.out   >   gprof.out

cd ../ANattr
bin/$compiler/profile/nodeattrtest
gprof bin/$compiler/profile/nodeattrtest gmon.out   >   gprof.out

cd ../ANode
bin/$compiler/profile/nodetest
gprof bin/$compiler/profile/nodetest gmon.out   >   gprof.out

cd ../AParser
bin/$compiler/profile/tparser
gprof bin/$compiler/profile/tparser gmon.out   >   gprof.out

cd ../Base
bin/$compiler/profile/basetest
gprof bin/$compiler/profile/basetest gmon.out   >   gprof.out

cd ../Client
bin/$compiler/profile/tclient
gprof bin/$compiler/profile/tclient gmon.out   >   gprof.out

cd ../Server
bin/$compiler/profile/tserver
gprof bin/$compiler/profile/tserver gmon.out   >   gprof.out

cd ../Test
bin/$compiler/profile/test
gprof bin/$compiler/profile/test gmon.out   >   gprof.out

cd ../Simulator
bin/$compiler/profile/testsimulator
gprof bin/$compiler/profile/testsimulator gmon.out   >   gprof.out

cd ../AParser
bin/$compiler/profile/tsingle
gprof bin/$compiler/profile/tsingle gmon.out   >   gprof.out
