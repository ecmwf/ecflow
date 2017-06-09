#!/bin/sh

## Copyright 2009-2017 ECMWF.
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
cd $WK/ACore
bin/$compiler/profile/u_acore
gprof bin/$compiler/profile/u_acore   gmon.out   >   gprof.out

cd $WK/ANattr
bin/$compiler/profile/u_anattr
gprof bin/$compiler/profile/u_anattr gmon.out   >   gprof.out

cd $WK/ANode
bin/$compiler/profile/u_anode
gprof bin/$compiler/profile/u_anode gmon.out   >   gprof.out

cd $WK/ANode/parser
bin/$compiler/profile/u_aparser
gprof bin/$compiler/profile/u_aparser gmon.out   >   gprof.out

cd $WK/Base
bin/$compiler/profile/u_base
gprof bin/$compiler/profile/u_base gmon.out   >   gprof.out

cd $WK/Client
bin/$compiler/profile/s_client
gprof bin/$compiler/profile/s_client gmon.out   >   gprof.out

cd $WK/Server
bin/$compiler/profile/u_server
gprof bin/$compiler/profile/u_server gmon.out   >   gprof.out

cd $WK/Test
bin/$compiler/profile/test
gprof bin/$compiler/profile/test gmon.out   >   gprof.out

cd $WK/CSim
bin/$compiler/profile/c_csim
gprof bin/$compiler/profile/c_csim gmon.out   >   gprof.out

cd $WK/ANode/parser
bin/$compiler/profile/perf_aparser
gprof bin/$compiler/profile/perf_aparser gmon.out   >   gprof.out
