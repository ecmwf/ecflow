#/bin/bash
## Copyright 2009-2020 ECMWF.
## This software is licensed under the terms of the Apache Licence version 2.0 
## which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
## In applying this licence, ECMWF does not waive the privileges and immunities 
## granted to it by virtue of its status as an intergovernmental organisation 
## nor does it submit to any jurisdiction. 

# GDB setup:
# https://www.thomasvitale.com/how-to-setup-gdb-and-eclipse-to-debug-c-files-on-macos-sierra/
#
# Using lldb-mi debugger
#https://wiki.eclipse.org/CDT/User/FAQ#How_do_I_get_the_LLDB_debugger.3F
#

# ====================================================================================
# Build type to Release    
cmake_build_type=Release
if [[ $# -eq 1 ]] ; then
   if [[ $1 = debug ]] ; then
       cmake_build_type=Debug
   fi
fi

clang_arg=

CXX_FLAGS="-fvisibility=hidden -fvisibility-inlines-hidden -Wno-deprecated-declarations"
#CXX_LINK_FLAGS=""

cmake_extra_options=""
if [[ "$clang_arg" = clang ]] ; then
    # relies on brew install
    #brew install cmake
    #
    # the brew version of boost was built with -fvisibility=hidden -fvisibility-inlines-hidden
    # We need same flags, otherwise large warning messages
    #brew install boost
    #brew install boost-python3
    #
    #https://medium.com/@timmykko/using-openssl-library-with-macos-sierra-7807cfd47892
    #brew install openssl  # however this may not set the right links
    #
    # Do the following if config steps fails with cannot find openssl
    #ln -s /usr/local/opt/openssl/include/openssl /usr/local/include
    #
    #ln -s /usr/local/opt/openssl/lib/libssl.1.1.1.dylib /usr/local/lib/
    #
    # to list the clang default system search path use:
    #clang -x c -v -E /dev/null
   cmake_extra_options="$cmake_extra_options -DBOOST_ROOT=/usr/local"
   CXX_FLAGS="$CXX_FLAGS -ftemplate-depth=512"
else
   # brew install gcc
   # manually installed boost, built with brew gcc
   cmake_extra_options="$cmake_extra_options -DBOOST_ROOT=${HOME}/boost/boost_1_72_0"
   cmake_extra_options="$cmake_extra_options -DCMAKE_C_COMPILER=/usr/local/opt/gcc/bin/gcc-9"
   cmake_extra_options="$cmake_extra_options -DCMAKE_CXX_COMPILER=/usr/local/opt/gcc/bin/g++-9"
fi

cmake ${HOME}/git/ecflow \
      -DCMAKE_MODULE_PATH=${HOME}/git/ecbuild/cmake \
      -DCMAKE_CXX_FLAGS="$CXX_FLAGS" \
      -DCMAKE_BUILD_TYPE=$cmake_build_type \
      -DCMAKE_PREFIX_PATH=/usr/local/opt/qt \
      -DCMAKE_INSTALL_PREFIX=${HOME}/install_test \
      -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
      ${cmake_extra_options}
      
      #-DCMAKE_EXE_LINKER_FLAGS="$CXX_LINK_FLAGS" \
      
      
      