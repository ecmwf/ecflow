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

# ====================================================================
show_error_and_exit() {
   echo "cmake.sh expects at least one argument"
   echo " cmake.sh [ options] "
   echo ""
   echo "   debug          - make a DEBUG build"
   echo "   make           - run make after cmake"
   echo "   ctest          - all ctest -R <test> -V"
   echo "   clang          - build with clang compiler"
   echo "   no_gui         - Don't build the gui"
   echo "   no_ssl         - build without using openssl"
   echo "   log            - enable debug output"
   exit 1
}

# ====================================================================================
# Build type to Release    
mode_arg=release
compiler=clang

make_arg=
test_arg=
ctest_arg=
no_gui_arg=
no_ssl_arg=
log_arg=
clean_arg=
install_arg=
asan_arg=
msan_arg=
ubsan_arg=


while [[ "$#" != 0 ]] ; do   

   if [[ "$1" = debug || "$1" = release ]] ; then
      mode_arg=$1
   elif  [[ "$1" = make ]] ; then
      make_arg=$1
      shift
      while [[ "$#" != 0 ]] ; do
         make_arg="$make_arg $1"
         shift
      done
      break
   elif [[ "$1" = clean ]] ;   then clean_arg=$1 ;
   elif [[ "$1" = install ]] ; then install_arg=$1 ;
   elif [[ "$1" = gcc ]] ;     then compiler=$1 ;
   elif [[ "$1" = clang ]] ;   then compiler=$1 ;
   elif [[ "$1" = no_gui ]] ;  then no_gui_arg=$1 ;
   elif [[ "$1" = no_ssl ]] ;  then no_ssl_arg=$1 ;
   elif [[ "$1" = log ]]   ;   then log_arg=$1 ;
   elif [[ "$1" = test ]] ;    then test_arg=$1 ;
   elif [[ "$1" = asan ]]  ;   then asan_arg=$1 ;
   elif [[ "$1" = msan ]]  ;   then msan_arg=$1 ;
   elif [[ "$1" = ubsan ]]  ;  then ubsan_arg=$1 ;
   elif [[ "$1" = ctest ]] ;   then  
      ctest_arg=$1 ; 
      shift
      while [[ "$#" != 0 ]] ; do
         ctest_arg="$ctest_arg $1"
         shift
      done
      break
   else
     show_error_and_exit
   fi

   # shift remove last argument
   shift
done


# ====================================================================================
# default to Release  
cmake_build_type=
if [[ $mode_arg = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

CXX_FLAGS="-fvisibility=hidden -fvisibility-inlines-hidden -Wno-deprecated-declarations"
CXX_LINK_FLAGS=""

cmake_extra_options=""
if [[ "$compiler" = clang ]] ; then
    # relies on brew install
    # brew install cmake
    # brew install qt
    #
    # the brew version of boost was built with -fvisibility=hidden -fvisibility-inlines-hidden
    # We need same flags, otherwise large warning messages
    # brew install boost
    # brew install boost-python3
    #
    # https://medium.com/@timmykko/using-openssl-library-with-macos-sierra-7807cfd47892
    # brew install openssl  # however this may not set the right links
    #
    # Do the following if config steps fails with cannot find openssl
    # ln -s /usr/local/opt/openssl/include/openssl /usr/local/include
    #
    # ln -s /usr/local/opt/openssl/lib/libssl.1.1.1.dylib /usr/local/lib/
    #
    # to list the clang default system search path use:
    # clang -x c -v -E /dev/null
   cmake_extra_options="$cmake_extra_options -DBOOST_ROOT=/usr/local"
   CXX_FLAGS="$CXX_FLAGS -ftemplate-depth=512"
else
   # brew install gcc
   # manually installed boost, built with brew gcc
   cmake_extra_options="$cmake_extra_options -DBOOST_ROOT=${HOME}/boost/boost_1_72_0"
   cmake_extra_options="$cmake_extra_options -DCMAKE_C_COMPILER=/usr/local/opt/gcc/bin/gcc-9"
   cmake_extra_options="$cmake_extra_options -DCMAKE_CXX_COMPILER=/usr/local/opt/gcc/bin/g++-9"
fi

if [[ "$asan_arg" = asan ]] ; then

   CXX_FLAGS="$CXX_FLAGS -fsanitize=address -fno-omit-frame-pointer"
   CXX_LINK_FLAGS="$CXX_LINK_FLAGS -fsanitize=address"
   export ECF_TEST_SANITIZER_AS=1  # enable address sanitizer tests
   
   #cmake_extra_options="$cmake_extra_options -CMAKE_CXX_LINK_EXECUTABLE= "
   #cmake -DCMAKE_LINKER=/path/to/linker -DCMAKE_CXX_LINK_EXECUTABLE="<CMAKE_LINKER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
   #cmake_extra_options="$cmake_extra_options -DCMAKE_BUILD_TYPE=ASAN"
   cmake_extra_options="$cmake_extra_options -DCMAKE_LINKER=/Library/Developer/CommandLineTools/usr/bin/clang"
fi
if [[ "$ubsan_arg" = ubsan ]] ; then
   #CXX_FLAGS="$CXX_FLAGS -fsanitize=undefined -fsanitize-minimal-runtime -fno-omit-frame-pointer -fno-sanitize-recover=all" # exit after all errors minimal info
   #CXX_FLAGS="$CXX_FLAGS -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-recover=all"                           # exit on first error, max info
   CXX_FLAGS="$CXX_FLAGS -fsanitize=undefined -fno-omit-frame-pointer"                                                     # report error, max info, no exit on error
   CXX_LINK_FLAGS="$CXX_LINK_FLAGS -fsanitize=undefined"
   export ECF_TEST_SANITIZER_UB=1  # enable undefined behaviour tests
   cmake_extra_options="$cmake_extra_options -DCMAKE_LINKER=/Library/Developer/CommandLineTools/usr/bin/clang"
fi
if [[ "$msan_arg" = msan ]] ; then
   CXX_FLAGS="$CXX_FLAGS -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins"
   CXX_LINK_FLAGS="$CXX_LINK_FLAGS -fsanitize=memory"
   cmake_extra_options="$cmake_extra_options -DCMAKE_LINKER=/Library/Developer/CommandLineTools/usr/bin/clang"
fi

log_options=
if [[ $log_arg = log ]] ; then
    log_options="-DECBUILD_LOG_LEVEL=DEBUG"
fi

gui_options=
if [[ $no_gui_arg = no_gui ]] ; then
    gui_options="-DENABLE_UI=OFF"
fi

ssl_options="-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl"
if [[ $no_ssl_arg = no_ssl ]] ; then
    ssl_options="-DENABLE_SSL=OFF"
fi

test_options=
if [[ $test_arg = test ]] ; then
   test_options="-DENABLE_ALL_TESTS=ON"
fi

bdir=${HOME}/git/bdir/ecflow/$mode_arg/$compiler
if [[ $clean_arg = clean ]] ; then
   rm -rf ${HOME}/git/bdir
fi
mkdir -p $bdir
cd $bdir

if [[ "$ctest_arg" != "" ]] ; then

    if [[ "$asan_arg" = asan ]] ; then
      ASAN_OPTIONS=detect_leaks=1
    fi
    
    $ctest_arg 
    exit 0
fi


# ====================================================================================
# Use for local install
version=$(awk '/^project/ && /ecflow/ && /VERSION/ {for (I=1;I<=NF;I++) if ($I == "VERSION") {print $(I+1)};}' ${HOME}/git/ecflow/CMakeLists.txt) 
#echo $version

# ====================================================================================
# default to install in $HOME/install, otherwise if install passed, /usr/local/opt  
install_prefix=${HOME}/install/ecflow/${version}
if [[ $install_arg = install ]] ; then
    install_prefix=/usr/local/opt/ecflow/${version}
fi

cmake ${HOME}/git/ecflow \
      -DCMAKE_MODULE_PATH=${HOME}/git/ecbuild/cmake \
      -DCMAKE_CXX_FLAGS="$CXX_FLAGS" \
      -DCMAKE_EXE_LINKER_FLAGS="$CXX_LINK_FLAGS" \
      -DCMAKE_BUILD_TYPE=$cmake_build_type \
      -DCMAKE_PREFIX_PATH=/usr/local/opt/qt \
      -DCMAKE_INSTALL_PREFIX=${install_prefix} \
      ${cmake_extra_options} \
      ${gui_options} \
      ${ssl_options} \
      ${log_options} \
      ${test_options}  
      
# =============================================================================================
if [[ "$make_arg" != "" ]] ; then
    $make_arg 
fi

