#!/bin/bash

## Copyright 2009-2020 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

set -x
#set -u

# Only uncomment for debugging this script
#rm -rf stage
#rm -rf tmpBuildDir
#rm -rf bin.v2

# ===============================================================
# allow tool to be overridden
tool=gcc
tool_path=""
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

# =================================================================================================
# LAYOUT
# --layout=system    -> libboost_system.a (default)
# --layout=tagged    -> libboost_system-mt-d-x86.a(debug)      libboost_system-mt-x86.a(release) 
# --layout=versioned -> libboost_system-xlc-mt-d-1.42(debug)   libboost_system-xlc-mt-1_42.a(release)
#
# for some reason on cray versioned does not embed the compiler name as a part
# of the library name. However it does add the boost version.
# Hence we will use this to distinguish between the g++ and cray boost libs
# On *CRAY* we can have 3 compilers we will use the versioned for CRAY and INTEL library
# 
# https://gitlab.kitware.com/cmake/cmake/issues/18908
# FROM boost 1.69 'layout=tagged' adds -x86 to the library names
# Hence make sure top level CMakeList.txt adds:
# set(Boost_ARCHITECTURE       "-x64") # from boost 1.69 layout=tagged adds libboost_system-mt-x64.a
# 
layout=system

CXXFLAGS=-d2     # dummy argument, since CXXFLAGS is quoted
CXXFLAGS=cxxflags=-fPIC

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
      		#module load intel/19.0.4
  
      		cp $WK/build_scripts/site_config/site-config-Linux64-intel.jam $SITE_CONFIG_LOCATION 

  	  elif [ $tool = clang ] ; then
      		# module unload gnu
      		# module load clang/7.0.1
  
      		cp $WK/build_scripts/site_config/site-config-Linux64-clang.jam $SITE_CONFIG_LOCATION 
       		CXXFLAGS=cxxflags="-fPIC -ftemplate-depth=1024 -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wno-unused-variable"
  	  fi
   fi
     
  else 
    cp $WK/build_scripts/site_config/site-config-Linux.jam $SITE_CONFIG_LOCATION
  fi
  
elif test_uname Darwin ; then

   cp $WK/build_scripts/site_config/site-config-Darwin.jam $SITE_CONFIG_LOCATION 
   tool_path="/usr/local/opt/gcc/bin/gcc-9"
   CXXFLAGS=cxxflags="-fPIC -Wunused-function -Wno-maybe-uninitialized -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wno-unused-variable -Wno-parentheses"
   
elif test_uname HP-UX ; then

  tool=acc
  cp $WK/build_scripts/site_config/site-config-HPUX.jam $SITE_CONFIG_LOCATION
   
elif test_uname AIX ; then

   # on c1a
   tool=vacpp
   cp $WK/build_scripts/site_config/site-config-AIX.jam $SITE_CONFIG_LOCATION
fi


# ===============================================================
# USER-CONFIG.JAM
#
# *** placing using gcc, in user-config.jam otherwise errors in building python ****
#
# using toolset-name : version :invocation-command : options ;
#   where options allows <cflags, cxxflags, compileflags and linkflags >
#
if [[ ! -e $HOME/user-config.jam ]] ; then
   cp $BOOST_ROOT/tools/build/example/user-config.jam $HOME/.
   echo "# On linux 64, because most of the static library's, are placed in a shared libs(ecflow.so)" >> $HOME/user-config.jam
   echo "# hence we need to compile with -fPIC"                                                       >> $HOME/user-config.jam
   echo "using $tool : : $tool_path : <cxxflags>-fPIC ;"                                              >> $HOME/user-config.jam
fi

#
# Note: if '--build-dir=./tmpBuildDir' is omitted, boost will build the libs in a directory:
#   bin.v2/
# On completion, the library is copied to:
#   stage/lib/
# 

# We use tagged as that allows the debug and release builds to built together, if required
#
echo "using compiler $tool with release variants"
 
if [[ ${BOOST_NUMERIC_VERSION} -le 1690 ]] ; then
   # boost system is header only from boost version 1.69, stub library built for compatibility
   ./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-system variant=release -j4
fi
./b2 --debug-configuration --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-date_time  variant=release -j4
./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-filesystem variant=release -j4
./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-program_options variant=release -j4
./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-test   variant=release -j4
./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-timer  variant=release -j4
./b2 --build-dir=./tmpBuildDir toolset=$tool "$CXXFLAGS" stage link=static --layout=$layout --with-chrono variant=release -j4


# Allow python to be disabled  
if [ -n "$ECF_NO_PYTHON" ] ; then   
   echo "****************************************************************************"
   echo "Ignore boost python. ECF_NO_PYTHON set."
   echo "****************************************************************************"
else
   # ================================================================================
   echo Build python
   # ================================================================================
   #*** If the boost python HAS not been built, and we build in $WK/Pyext, then it will build 
   #*** boost python in $BOOST_ROOT/bin.v2/
   #*** It appears to build boost python single threaded. (i.e you do not see threading-multi) in the directory path.
   
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
   #
   # for 2/ use    
   #    We use the python snippet below to work out the correct include path
   #    This is because ./bootstrap.sh --with-python=python3 
   #    does not configure the python correctly
   #    if all else fails comment ot the python snippet below and use:
   #    export CPLUS_INCLUDE_PATH="$CPLUS_INCLUDE_PATH:<path to pythn includes"
   # 
   # When installing BOOST-python libs, make sure to call module load python3 *FIRST*
   # Otherwise it will pick the python specified in project-config.jam, which may not be correct
   #
   # ==========================================================================================
   # PYTHON3:
   # BOOST < 1.67
   #   To build BOTH python2 and Python 3 libraries, the order is important 
   #   - First build python3 and then python2. This is because in boost 1.53 not all python libs have the 3 tag.
   #   Build:
   #   0/ ./b2 --with-python --clean # clean any previous python build. VERY IMPORTANT
   #   1/ module load python3, this update the $PATH
   #   2/ ./bootstrap.sh --with-python=python3
   #   3/ Need to manually edit $BOOST_ROOT/project-config.jam,  make sure file '$BOOST_ROOT/project-config.jam' has:
   #      Can be generated with:
   #      > python3 -c "import sys;from sysconfig import get_paths;print('using python : ','{0}.{1}'.format(sys.version_info[0], sys.version_info[1]),' : ',get_paths()['data'],' : ',get_paths()['include'])"
   #      verify path is correct using: python3.6-config --includes
   #     
   #      # using python : 2.7 : "/usr/local/apps/python/2.7.15-01" ;
   #      # using python 
   #      # : 3.6 
   #      # : /usr/local/apps/python3/3.6.8-01/bin/python3  # ***** If this is a prefix dir, includes get messed up, have mix of python2 & 3
   #      # : /usr/local/apps/python3/3.6.8-01/include/python3.6m # include directory
   #      # ;
   #      using python 
   #       : 3.7 
   #       : /usr/local/apps/python3/3.7.1-01/bin/python3  # ***** If this is a prefix dir, includes get messed up, have mix of python2 & 3
   #       : /usr/local/apps/python3/3.7.1-01/include/python3.7m # include directory
   #       ;  
   #       ...
   #      option.set includedir : /usr/local/apps/python3/3.5.1-01/include/python3.5m ;  # ***MAKE*** sure this is set
   #
   #     ***** cmd/prefix must be path to python3, otherwise compilation include files has a mixture of
   #     python 2.7 and 3.6, YUK, took ages to debug
   #
   #   Python 2:
   #     0/ ./b2 --with-python --clean   # Clean previous build
   #     1/ module load python2
   #     2/ ./bootstrap.sh --with-python=/path/to/python2.7
   #     3/ invoke this script
   #
   #   Check:
   #     To check the build make sure we don't have symbol pulled in from python2 libs
   #     cd $BOOST_ROOT/stage/lib
   #     nm -D *python* | grep PyClass_Type                                                 # PyClass_Type is a symbol *ONLY* used in python2.x
   #     nm -D  /tmp/ma0/workspace/bdir/release/ecflow/Pyext/ecflow.so | grep PyClass_Type  # check ecflow.so
   #
   # BOOST 1.67 >=
   #   we can now use
   #     ./b2 python=2.7,3.6,3.7 ....
   #   to build all the python variants, providing project-config.jam *has* multiple 'using python' statements
   #     using python : 2.7 : "/usr/local/apps/python/2.7.15-01" ;
   #     using python : 3.6 : /usr/local/apps/python3/3.6.8-01/bin/python3 : /usr/local/apps/python3/3.6.8-01/include/python3.6m ;
   #     using python : 3.7 : /usr/local/apps/python3/3.7.1-01/bin/python3 : /usr/local/apps/python3/3.7.1-01/include/python3.7m ;  
   # ===========================================================================================================

   # ===========================================================================================================
   # Attempt at replacing 'using python' with the correct python include dir in site-config.jam
   # ===========================================================================================================
   python_file=compute_python_using_statement.py

cat << EOF > $python_file
import sys
from sysconfig import get_paths
python_version = "{0}.{1}".format(sys.version_info[0], sys.version_info[1])
python_path_info = get_paths()
python_exe = sys.executable
python_include = python_path_info['include']
using_python = '   using python : ' + python_version  + ' : ' + python_exe  + ' : ' + python_include  + ' ;'
print(using_python)
EOF

   echo "ECF_PYTHON2 = [ os.environ ECF_PYTHON2 ] ;"  >>  $SITE_CONFIG_LOCATION
   
   which python3
   if [ "$?" = "0" ] ; then
      
      python_version=$(python3 -c 'import sys;print(sys.version_info[0],sys.version_info[1],sep="")')
      if [[ ${BOOST_NUMERIC_VERSION} -le 1670 ]] ; then
         python_version=3
      fi
      
      python_dot_version=$(python3 -c 'import sys;print(sys.version_info[0],".",sys.version_info[1],sep="")')
      echo 'if ! $(ECF_PYTHON2) {'                                                       >> $SITE_CONFIG_LOCATION
      echo "   lib boost_python : : <file>\$(BOOST_ROOT)/stage/lib/libboost_python${python_version}.a ;" >> $SITE_CONFIG_LOCATION
      python3 $python_file                                                               >> $SITE_CONFIG_LOCATION
      echo '}'                                                                           >> $SITE_CONFIG_LOCATION
      echo "constant PYTHON3_VERSION : $python_dot_version ;"                            >> $SITE_CONFIG_LOCATION
      
      ./b2 --with-python --clean     
      ./b2 --debug-configuration toolset=$tool link=shared,static variant=release "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j4
   fi

   which python
   if [ "$?" = "0" ] ; then
      export ECF_PYTHON2=1 # so that we use ' using python ......'
      echo 'if $(ECF_PYTHON2) {'                                                         >> $SITE_CONFIG_LOCATION
      
      if [[ ${BOOST_NUMERIC_VERSION} -le 1670 ]] ; then
         echo '   lib boost_python : : <file>$(BOOST_ROOT)/stage/lib/libboost_python.a ;' >> $SITE_CONFIG_LOCATION
      else
         echo '   lib boost_python : : <file>$(BOOST_ROOT)/stage/lib/libboost_python27.a ;' >> $SITE_CONFIG_LOCATION
      fi
      python $python_file                                                                >> $SITE_CONFIG_LOCATION
      echo '}'                                                                           >> $SITE_CONFIG_LOCATION
                                                                            
      ./b2 --with-python --clean     
      ./b2  --debug-configuration toolset=$tool link=shared,static variant=release "$CXXFLAGS" stage --layout=$layout threading=multi --with-python -d2 -j4
   fi
   
   rm $python_file
fi
