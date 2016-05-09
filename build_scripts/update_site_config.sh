#!/bin/sh

## Copyright 2009-2016 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This file is used build the boost libs used by ecflow
# This script Use $BOOST_ROOT and $WK environment variable

echo "WK=$WK"
echo "BOOST_ROOT=$BOOST_ROOT"

#
# From boost 1.56 > the location of site-config.jam location has changed
#
SITE_CONFIG_LOCATION=$BOOST_ROOT/tools/build/v2/site-config.jam
BOOST_VERSION="$(basename $BOOST_ROOT)"
if [[ "$BOOST_VERSION" = boost_1_56_0 || "$BOOST_VERSION" = boost_1_57_0 ]] ; then
   SITE_CONFIG_LOCATION=$BOOST_ROOT/tools/build/src/site-config.jam
fi

tool=

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
  tool=gcc
  X64=$(uname -m)
  if [ "$X64" = x86_64 ]
  then
    # PE_ENV is defined in cray environment, at least on sandy bridge
    if [ "$PE_ENV" = GNU -o "$PE_ENV" = INTEL -o "$PE_ENV" = CRAY ]
    then
       CXXFLAGS=cxxflags=-fPIC
       layout=versioned  
       
       cp $WK/build_scripts/site_config/site-config-cray.jam $SITE_CONFIG_LOCATION
       if [ "$PE_ENV" = INTEL ] ; then
          tool=intel
       fi
       if [ "$PE_ENV" = CRAY ] ; then
          tool=cray
       fi
    else
       cp $WK/build_scripts/site_config/site-config-Linux64.jam $SITE_CONFIG_LOCATION  
    fi
     
  else 
    cp $WK/build_scripts/site_config/site-config-Linux.jam $SITE_CONFIG_LOCATION
  fi
  
elif test_uname HP-UX ; then

  tool=acc
  cp $WK/build_scripts/site_config/site-config-HPUX.jam $SITE_CONFIG_LOCATION
   
elif test_uname AIX ; then

   # on c1a
   tool=vacpp
   cp $WK/build_scripts/site_config/site-config-AIX.jam $SITE_CONFIG_LOCATION
 
fi

 