#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

cd $WK

# Remove the bin directories
rm -rf ACore/bin
rm -rf ANattr/bin
rm -rf ANode/bin 
rm -rf AParser/bin 
rm -rf Base/bin
rm -rf CSim/bin
rm -rf Client/bin 
rm -rf Server/bin 
rm -rf Test/bin
rm -rf TestEcfSms/bin 
rm -rf Pyext/bin 
rm -rf view/bin 
   
# remove generated files
rm -rf Doc/online/_build/*
rm -rf Test/data/ECF_HOME_debug*
rm -rf Test/data/ECF_HOME_release*
rm -rf view/data/ECF_HOME_debug*
rm -rf view/data/ECF_HOME_release*
rm -rf AParser/test/data/single_defs/mega.def_log
rm -rf Pyext/test.def
rm -rf Pyext/build
rm -rf bin
rm -rf RemoteSystemsTempFiles
rm -rf *.dat
rm -rf *.log

find . -name \*~ -exec rm -rf \*~ {} \; -print
find . -name \*.mk -exec rm -rf \*.mk {} \; -print
find . -name \*.so -exec rm -rf \*.so {} \; -print
find . -name \*.tmp -exec rm -rf \*.tmp {} \; -print
find . -name \*.job\* -exec rm -rf \*.job\* {} \; -print
find . -name \*.check -exec rm -rf \*.check {} \; -print
find . -name \*.flat -exec rm -rf \*.flat {} \; -print
find . -name \*.depth -exec rm -rf \*.depth {} \; -print
find . -name \*.out -exec rm -rf \*.out {} \; -print
find . -name \*.pyc -exec rm -rf \*.pyc {} \; -print
find . -name t\*.1 -exec rm -rf t\*.1 {} \; -print
find . -name gmon.out -exec rm -rf gmon.out {} \; -print
find . -name gnuplot.dat -exec rm -rf gnuplot.dat {} \; -print
find . -name gnuplot.script -exec rm -rf gnuplot.script {} \; -print
find . -name ecflow.html -exec rm -rf ecflow.html {} \; -print
find . -name core -exec rm -rf core {} \; -print
find . -name `hostname`.*.ecf.* -exec rm -rf `hostname`.*.ecf.* {} \; -print
find . -name callgrind.out.\* -exec rm -rf callgrind.out.\* {} \; -print
find . -name massif.out.\* -exec rm -rf massif.out.* {} \; -print

# remove any defs file at the workspace level. There should not be any
rm -rf *.def
   
# Remove any lock file create by tests which used EcfPortLock.hpp
rm -rf *.lock

