#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# untar up ecflow 

rm -f incr.tar
gunzip incr.tar.gz

# The original file are read only, hence remove first or extraction fails
tar -tf incr.tar | xargs rm -f 

# Untar the changes
tar -xvf incr.tar
