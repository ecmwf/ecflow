#!/bin/ksh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 


# cat ecflowview.menu | menu2c.sh  > ecflowview.menu.h
while read -r line ; do
    l=$(echo $line | sed -e 's:":\\":gi')
  echo "(char*) \" $l \","
done
echo NULL
