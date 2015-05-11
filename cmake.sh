#!/bin/ksh

# ====================================================================
show_error_and_exit() {
   echo "cmake.sh expects at least one argument, note 'san' is short for thread sanitiser i.e"
   echo "  cmake.sh debug [clang] [san]"
   echo "  cmake.sh release [clang] [san]"
   exit 1
}

if [[ "$#" -eq 0 || "$#" -gt 3 ]] ; then
	show_error_and_exit
fi
if [[ "$1" != debug && "$1" != release ]] ; then
   show_error_and_exit
fi
if [[ "$#" -ge 2 && "$2" != clang ]] ; then
   show_error_and_exit
fi
if [[ "$#" -ge 3 && "$3" != san ]] ; then
   show_error_and_exit
fi

# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

# ====================================================================
cmake_extra_options=""
if [[ "$2" = clang ]] ; then
	module unload gnu
	module load clang
	cmake_extra_options="-DCMAKE_CXX_FLAGS=-ftemplate-depth=512"
fi
if [[ "$3" = san ]] ; then
	module uload gnu
	module load clang
	cmake_extra_options="$cmake_extra_options -DCMAKE_C_FLAGS=-fsanitize=thread"
fi

# ===================================================================
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

rm -rf ecbuild/$1
mkdir -p ecbuild/$1
cd ecbuild/$1
      
cmake_build_type=
if [[ $1 = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

# ==========================================================================================================
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
make -j8
ctest -R ^u_
ctest -R c_
ctest -R py_u
ctest -R py_s
ctest -R s_


# ===============================================================
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