#!/bin/sh

## Copyright 2009-2016 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This file is used paths up version of the boost libs
# This script Use $BOOST_ROOT and $WK environment variable
# Assumes boost version 1.48

# Check that a command is in the PATH.
test_path ()
{
    if `command -v command 1>/dev/null 2>/dev/null`; then
        command -v $1 1>/dev/null 2>/dev/null
    else
        hash $1 1>/dev/null 2>/dev/null
    fi
}

test_uname ()
{
    if test_path uname; then
        test `uname` = $*
    fi
}

if test_uname Linux ; then

   echo "Nothing to fix"
  
elif test_uname HP-UX ; then

  # Hack for utf8_codecvt_facet due to compiler build error on ACC
  cp $WK/build_scripts/hpux_fix/utf8_codecvt_facet.cpp  $BOOST_ROOT/libs/detail/

  # Hack because boost::int8_t was being interpreted as a char,
  # This cause duplicate class definition during template instantiation.
  # in the file ./boost/numeric/conversion/detail/preprocessed/numeric_cast_traits.hpp
  # See:: https://svn.boost.org/trac/boost/attachment/ticket/6158/cstdint_patch.diff
  cp $WK/build_scripts/hpux_fix/cstdint.hpp  $BOOST_ROOT/boost/

elif test_uname AIX ; then  
   
   # Fix bug where release version crashes due to bug in Serialization/compiler
   # See file $WK/build_scripts/aix_fix/README
   cp $WK/build_scripts/aix_fix/singleton.hpp $BOOST_ROOT/boost/serialization/
   cp $WK/build_scripts/aix_fix/force_include.hpp $BOOST_ROOT/boost/serialization/
fi
