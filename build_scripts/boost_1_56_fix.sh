#!/bin/sh

## Copyright 2009-2019 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This script Use $BOOST_ROOT and $WK environment variable
# Assumes boost version 1.56

# See: https://svn.boost.org/trac/boost/ticket/10348
cp $WK/build_scripts/fix/boost_1_56_0/shared_ptr_helper.hpp  $BOOST_ROOT/boost/serialization/.
