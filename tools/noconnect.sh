#!/bin/sh
# disconnect all hosts to start ecflowview with empty screen
#==========================================================================
##.TITLE   ECMWF utility for ecFlow
##.NAME     
##.SECTION ECFLOW
##.AUTHOR   
## Revision    : $Revision: #7 $ 
##
## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 
##
##.FILE     
##.INFO     
#==========================================================================

d=$HOME/.xcdprc
d=$HOME/.ecflowrc
for f in $d/*options
do
    # echo "##  $f"
    if [ `grep "connect:true" $f 2>/dev/null` ]
	then
	# echo "### $f"
	sed -e 's|connect:true|connect:false|' $f > ${f}.tmp
	mv ${f}.tmp $f
	fi
done
