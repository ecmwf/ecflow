#!/bin/ksh
# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ====================================================================
show_error_and_exit() {
   echo "cmake.sh expects at least one argument, note 'san' is short for thread sanitiser i.e"
   echo "  cmake.sh debug || release [clang] [san] [make] [verbose] [test]"
   exit 1
}

make_arg=
test_arg=
clang_arg=
clang_sanitiser_arg=
mode_arg=
verbose_arg=
while [[ "$#" != 0 ]] ; do   
   if [[ "$1" = debug || "$1" = release ]] ; then
      mode_arg=$1
   elif  [[ "$1" = clang ]] ; then
      clang_arg=$1
   elif  [[ "$1" = san ]] ; then
      clang_sanitiser_arg=$1
   elif  [[ "$1" = make ]] ; then
      make_arg=$1
   elif  [[ "$1" = verbose ]] ; then
      verbose_arg=$1
   else
   	 show_error_and_exit
   fi

   # shift remove last argument
   shift
done

if [ ${#mode_arg} -eq 0 ] ; then
   echo "cmake.sh expects mode i.e. debug or release"
   exit 1
fi


# ====================================================================
# To load module automatically requires Korn shell, system start scripts
# auto adds ability to module load
cmake_extra_options=""
if [[ "$clang_arg" = clang ]] ; then
	module unload gnu
	module load clang
	cmake_extra_options="-DCMAKE_CXX_FLAGS=-ftemplate-depth=512"
fi
if [[ "$clang_sanitiser_arg" = san ]] ; then
	module uload gnu
	module load clang
	cmake_extra_options="$cmake_extra_options -DCMAKE_C_FLAGS=-fsanitize=thread"
fi

# ====================================================================================
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

# ====================================================================================
rm -rf ecbuild/$mode_arg
mkdir -p ecbuild/$mode_arg
cd ecbuild/$mode_arg
    
# ====================================================================================  
cmake_build_type=
if [[ $mode_arg = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

# ====================================================================================
#
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
# -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#   *AND* for testing python install to local directory
#
# Use:DCMAKE_CXX_FLAGS to set compiler flags 
#            -DCMAKE_CXX_FLAGS="-ftemplate-depth-128  -finline-functions -Wno-inline -Wall -fPIC  -DNDEBUG" \

cmake ../.. -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake \
            -DCMAKE_BUILD_TYPE=$cmake_build_type \
            -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$release.$major.$minor \
            -DCMAKE_PYTHON_INSTALL_TYPE=local \
            ${cmake_extra_options}
            #-DCMAKE_PYTHON_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$release.$major.$minor/lib/python2.7/site-packages/ecflow
        
# =============================================================================================
if [[ $make_arg = make ]] ; then
	if [[ $verbose_arg = verbose ]] ; then
		make -j8 VERBOSE=1
	else
		make -j8
	fi
fi

# =============================================================================================
if [[ $test_arg = test ]] ; then
	ctest -R ^u_
	ctest -R c_
	ctest -R py_u
	ctest -R py_s
	ctest -R s_
fi

# ============================================================================================
# The process for thread checking very simple when using clang thread sanitiser
#*) Load the module for Clang
#module unload gnu
#module load clang
#
#*) Run cmake with the sanitizer
#cmake /path/to/source -DCMAKE_C_FLAGS="-fsanitize=thread" ...
#
#*) Compile and run tests
#make -jN
#ctest -jN
#
#If there are failures, the executable will fail with a non-zero exit code and a message on stderr e.g.
#WARNING: ThreadSanitizer: data race
#...
#WARNING: ThreadSanitizer: heap-use-after-free 