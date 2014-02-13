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

#
# --layout=system    -> libboost_system.a (default)
# --layout=tagged    -> libboost_system-mt-d.a(debug)          libboost_system-mt.a(release) 
# --layout=versioned -> libboost_system-xlc-mt-d-1.42(debug)   libboost_system-xlc-mt-1_42.a(release)
#
# for some reason on cray versioned does not embed the compiler name as a part
# of the library name. However it it does add the boost version.
# Hence we will use this to distinguish between the g++ and cray boost libs
# On *CRAY* we can have 3 compilers we will use the versioned for CRAY and INTEL library
layout=tagged

CXXFLAGS=
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
       
       cp $WK/build/site_config/site-config-cray.jam $BOOST_ROOT/tools/build/v2/site-config.jam
       if [ "$PE_ENV" = INTEL ] ; then
          tool=intel
       fi
       if [ "$PE_ENV" = CRAY ] ; then
          tool=cray
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
fi

# Only uncomment for debugging this script
#rm -rf stage
#rm -rf tmpBuildDir

#
# Note: if '--build-dir=./tmpBuildDir' is omitted, boost will build the libs in a directory:
#   bin.v2/
# On completion , the library is copied to:
#   stage/lib/
# 

# We use tagged as that allows the debug and release builds to built together
#
echo "using compiler $tool with build $1 variants "
# ========================================================================
# Note: boost thread *ONLY* need to test multi-threaded server See: define ECFLOW_MT
# ========================================================================
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-system variant=debug -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-date_time variant=debug -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-filesystem variant=debug  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-program_options variant=debug -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-serialization  variant=debug -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-test variant=debug  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-thread variant=debug  -j2


# ========================================================================
# Note: boost thread *ONLY* need to test multi-threaded server See: define ECFLOW_MT
# ========================================================================
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-system variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-date_time variant=release  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-filesystem variant=release   -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-program_options variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-serialization  variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-test variant=release  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool $CXXFLAGS stage link=static --layout=$layout --with-thread variant=release  -j2


# Allow python to be disabled  
if [ -n "$ECF_NO_PYTHON" ] ; then   
   echo "****************************************************************************"
   echo "Ignore boost python. ECF_NO_PYTHON set."
   echo "****************************************************************************"
else
   # ================================================================================
   # Build python
   # ================================================================================
   #*** If the boost python HAS not been built, and we build in $WK/Pyext, then it will build 
   #*** boost python in $BOOST_ROOT/bin.v2/
   #*** It appears to build boost python single threaded. (i.e you do not see threading-multi) in the directory path.
   #
   # To prebuild the boost python, hence we need to do the following: For now build both variants, keeps cmake happy! (i.e when finding libs)
   #
   ./bjam toolset=$tool link=shared variant=debug   $CXXFLAGS stage --layout=$layout threading=single --with-python -d2 -j2
   ./bjam toolset=$tool link=shared variant=release $CXXFLAGS stage --layout=$layout threading=single --with-python -d2 -j2
   ./bjam toolset=$tool link=shared variant=debug   $CXXFLAGS stage --layout=$layout threading=multi --with-python -d2 -j2
   ./bjam toolset=$tool link=shared variant=release $CXXFLAGS stage --layout=$layout threading=multi --with-python -d2 -j2
fi

 
