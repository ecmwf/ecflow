#!/bin/sh

## Copyright 2009-2012 ECMWF. 
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# This file is used build the boost libs used by ecflow
# This script Use $BOOST_ROOT and $WK environment variable

echo "WK=$WK"
echo "BOOST_ROOT=$BOOST_ROOT"

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
    if [[ "$PE_ENV" = GNU || "$PE_ENV" = INTEL || "$PE_ENV" = CRAY ]]
    then
       if [ "$PE_ENV" = GNU ] ; then
          cp $WK/build/site_config/site-config-Linux64.jam $BOOST_ROOT/tools/build/v2/site-config.jam  
       fi
       if [ "$PE_ENV" = INTEL ] ; then
         tool=intel
         cp $WK/build/site_config/site-config-Linux64-intel.jam $BOOST_ROOT/tools/build/v2/site-config.jam  
       fi
       if [ "$PE_ENV" = CRAY ] ; then
         tool=cray
         cp $WK/build/site_config/site-config-cray.jam $BOOST_ROOT/tools/build/v2/site-config.jam  
       fi
    else
       cp $WK/build/site_config/site-config-Linux64.jam $BOOST_ROOT/tools/build/v2/site-config.jam  
    fi
     
  else 
    cp $WK/build/site_config/site-config-Linux.jam $BOOST_ROOT/tools/build/v2/site-config.jam
  fi
  
elif test_uname HP-UX ; then

  tool=acc
  cp $WK/build/site_config/site-config-HPUX.jam $BOOST_ROOT/tools/build/v2/site-config.jam
   
elif test_uname AIX ; then

   # on c1a
   tool=vacpp
   cp $WK/build/site_config/site-config-AIX.jam $BOOST_ROOT/tools/build/v2/site-config.jam
 
   if test "$ARCH" = rs6000 
   then
     tool=vacpp
     cp $WK/build/site_config/site-config-AIX-rs6000.jam $BOOST_ROOT/tools/build/v2/site-config.jam
   
     # On ecgate however can't get gcc to work on AIX
     #tool=gcc
     #cp $WK/build/site_config/site-config-AIX-gcc.jam $BOOST_ROOT/tools/build/v2/site-config.jam
   fi
fi

 