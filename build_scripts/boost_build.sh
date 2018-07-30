#!/bin/sh

## Copyright 2009-2017 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# ===============================================================
# allow tool to be overridden
tool=gcc
if [ "$#" = 1 ] ; then   
	tool=$1
fi

# ===============================================================
# This file is used build the boost libs used by ecflow
# This script Use $BOOST_ROOT and $WK environment variable
echo "WK=$WK"
echo "BOOST_ROOT=$BOOST_ROOT"


# ===============================================================
# From boost 1.56 > the location of site-config.jam location has changed
#
SITE_CONFIG_LOCATION=$BOOST_ROOT/tools/build/v2/site-config.jam
BOOST_VERSION="$(basename $BOOST_ROOT)"                           # boost_1_53_0
BOOST_MAJOR_VERSION=$(echo $BOOST_VERSION | cut -d_ -f2)          # 1
BOOST_MINOR_VERSION=$(echo $BOOST_VERSION | cut -d_ -f3)          # 53
BOOST_PATCH_VERSION=$(echo $BOOST_VERSION | cut -d_ -f4)          # 0
BOOST_NUMERIC_VERSION=$(( 1000*BOOST_MAJOR_VERSION + 10*BOOST_MINOR_VERSION + BOOST_PATCH_VERSION ))

if [[ ${BOOST_NUMERIC_VERSION} -ge 1570 ]] ; then
   SITE_CONFIG_LOCATION=$BOOST_ROOT/tools/build/src/site-config.jam
fi


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

CXXFLAGS=-d2     # dummy argument, since CXXFLAGS is quoted
if test_uname Linux ; then
  X64=$(uname -m)
  if [ "$X64" = x86_64 ]
  then
    # PE_ENV is defined in cray environment, at least on sandy bridge
    if [ "$PE_ENV" = GNU -o "$PE_ENV" = INTEL -o "$PE_ENV" = CRAY ]
    then
       CXXFLAGS=cxxflags=-fPIC
       layout=versioned  
       tool=gcc
       cp $WK/build_scripts/site_config/site-config-cray.jam $SITE_CONFIG_LOCATION
       if [ "$PE_ENV" = INTEL ] ; then
          tool=intel
       fi
       if [ "$PE_ENV" = CRAY ] ; then
          tool=cray
       fi
    else
      if [ $tool = gcc ] ; then
  
      		cp $WK/build_scripts/site_config/site-config-Linux64.jam $SITE_CONFIG_LOCATION 
      		
            # for boost 1.53 and > gcc 4.3 -Wno-unused-local-typedefs  : not valid
            # for boost 1.53 and > gcc 4.8 -Wno-unused-local-typedefs  : get a lot warning messages , suppress
            # for boost 1.53 and > gcc 6.1 -Wno-deprecated-declarations: std::auto_ptr deprecated messages, suppress
            # for boost 1.53 and > gcc 6.3 c++14  
            compiler=$(gcc -dumpversion | sed 's/\.//g' )  # assume major.minor.patch
            if [ "$compiler" -gt 430  ] ; then
                CXXFLAGS=cxxflags="-fPIC -Wno-unused-local-typedefs"
       		fi
       		if [ "$compiler" -gt 610  ] ; then
       		   CXXFLAGS=cxxflags="-fPIC -Wno-unused-local-typedefs -Wno-deprecated-declarations"
       		fi
  	  elif [ $tool = intel ] ; then
      		#module unload gnu
      		#module load intel/15.0.2
  
      		cp $WK/build_scripts/site_config/site-config-Linux64-intel.jam $SITE_CONFIG_LOCATION 

  	  elif [ $tool = clang ] ; then
      		# module unload gnu
      		# module load clang/5.0.1
  
      		cp $WK/build_scripts/site_config/site-config-Linux64-clang.jam $SITE_CONFIG_LOCATION 
       		CXXFLAGS=cxxflags="-fPIC -ftemplate-depth=1024 -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wno-unused-variable"
  	  fi
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

# Only uncomment for debugging this script
#rm -rf stage
#rm -rf tmpBuildDir

#
# Note: if '--build-dir=./tmpBuildDir' is omitted, boost will build the libs in a directory:
#   bin.v2/
# On completion, the library is copied to:
#   stage/lib/
# 

# We use tagged as that allows the debug and release builds to built together
#
echo "using compiler $tool with build $1 variants "
# ========================================================================
# Note: boost thread *ONLY* need to test multi-threaded server See: define ECFLOW_MT
# ========================================================================
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-system variant=debug -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-date_time variant=debug -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-filesystem variant=debug  -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-program_options variant=debug -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-serialization  variant=debug -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-test variant=debug  -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-thread variant=debug  -j2
#./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-regex variant=debug  -j2   # ecflowUi
 


# ========================================================================
# Note: boost thread *ONLY* need to test multi-threaded server See: define ECFLOW_MT
# ========================================================================
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-system variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-date_time variant=release  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-filesystem variant=release   -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-program_options variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-serialization  variant=release -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-test variant=release  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-thread variant=release  -j2
./bjam --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-regex variant=release  -j2


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
   
    # ===============================================================================
    # Error to watch out for:
    # 1/ error: No best alternative for /python_for_extensions
    #    next alternative: required properties: <python>2.7 <target-os>linux
    #        matched
    #    next alternative: required properties: <python>2.7 <target-os>linux
    #        matched
    # 2/ pyconfig.h cant find include file:
    #
    # Note: ./bootstrap.sh will create a project-config.jam
    #
    # For both errors: Please check if you have more than one 'using python' in configuration files.
    # Please check site-config.jam, user-config.jam and project-config.jam and 
    # remove duplicated 'using python'.  Typically we remove $HOME/user-config.jam is using python is defined in it.
    # for 2/ use    
    #    export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:/usr/local/apps/python/2.7.12-01/include/python2.7/"
    # *ONLY* if first soultion fails ???
    # 
    #
    # When installing BOOST-python libs, make sure to call module load python *FIRST*
    # Otherwise it will pick the python specified in project-config.jam, which make not be correct
    #
    # ==========================================================================================
    # PYTHON3:
    # Build:
    #   0/ ./b2 --with-python --clean   # Clean previous build
    #   1/ module load python3, this update the $PATH
    #   2/ ./bootstrap.sh --with-python=/usr/local/apps/python3/3.5.1-01/bin/python3
    #   3/ Need to manually edit $BOOST_ROOT/project-config.jam,  make sure file '$BOOST_ROOT/project-config.jam' has:
    #
    #      using python 
    #       : 3.5 
    #       : /usr/local/apps/python3/3.5.1-01/bin/python3  # ***** If this is left as python3, includes get messed up, have mix of python2 & 3
    #       : /usr/local/apps/python3/3.5.1-01/include/python3.5m # include directory
    #       ; 
    #       ...
    #      option.set includedir : /usr/local/apps/python3/3.5.1-01/include/python3.5m ;  # ***MAKE*** sure this is set
    #
    #     ***** cmd/prefix must be path to python3, otherwise compilation include files has a mixture of
    #     python 2.7 and 3.5, YUK, took ages to debug
    #
    # Check:
    #   To check the build make sure we don't have symbol pulled in from python2 libs
    #   cd $BOOST_ROOT/stage/lib
    #   nm -D *python* | grep PyClass_Type                                                # PyClass_Type is a symbol *ONLY* used in python2.x
    #   nm -D /tmp/ma0/workspace/bdir/release/ecflow/Pyext/ecflow.so | grep PyClass_Type  # check ecflow.so
    # ===============================================================================
               
#   ./bjam toolset=$tool link=shared variant=debug   "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j2
#   ./bjam toolset=$tool link=static variant=debug   "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j2
   ./bjam toolset=$tool link=shared variant=release "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j2
   ./bjam toolset=$tool link=static variant=release "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j2
fi
