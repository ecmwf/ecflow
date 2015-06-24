#!/bin/ksh
# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable

# ====================================================================
show_error_and_exit() {
   echo "cmake.sh expects at least one argument"
   echo " cmake.sh debug || release [clang] [san] [make] [verbose] [test] [package_source] [debug]"
   echo "  "
   echo "   make           - run make after cmake"
   echo "   test           - run all the tests"
   echo "   new_viewer     - Build new viewer"
   echo "   test_safe      - only run deterministic tests"
   echo "   san            - is short for clang thread sanitiser"
   echo "   package_source - produces ecFlow-4.0.8-Source.tar.gz file, for users"
   echo "                    copies the tar file to $SCRATCH"
   echo "   copy_tarball   - copies ecFlow-4.0.8-Source.tar.gz to /tmp/$USER/tmp/. and untars file"
   exit 1
}

copy_tarball_arg=
package_source_arg=
make_arg=
test_arg=
test_safe_arg=
clang_arg=
clang_sanitiser_arg=
mode_arg=
verbose_arg=
new_viewer_arg=
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
   elif  [[ "$1" = package_source ]] ; then
      package_source_arg=$1
   elif  [[ "$1" = copy_tarball ]] ; then
      copy_tarball_arg=$1
   elif  [[ "$1" = test ]] ; then
      test_arg=$1
   elif  [[ "$1" = test_safe ]] ; then
      test_safe_arg=$1
   elif  [[ "$1" = new_viewer ]] ; then
      new_viewer_arg=$1
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

echo "copy_tarball_arg=$copy_tarball_arg"
echo "package_source_arg=$package_source_arg"
echo "make_arg=$make_arg"
echo "test_arg=$test_arg"
echo "test_safe_arg=$test_safe_arg"
echo "clang_arg=$clang_arg"
echo "clang_sanitiser_arg=$clang_sanitiser_arg"
echo "mode_arg=$mode_arg"
echo "verbose_arg=$verbose_arg"
set -x # echo script lines as they are executed

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
	module unload gnu
	module load clang
	cmake_extra_options="$cmake_extra_options -DCMAKE_C_FLAGS=-fsanitize=thread"
fi
if [[ "$new_viewer_arg" = new_viewer ]] ; then
	cmake_extra_options="$cmake_extra_options -DENABLE_VIEWER=ON"
fi

# ====================================================================================
# Use for local install
cd $WK
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

# ====================================================================================
rm -rf ../cmake_build_dir/ecflow/$mode_arg

# clean up source before packaging, do this after deleting ecbuild
if [[ $package_source_arg = package_source ]] ; then
	source $WK/build_scripts/clean.sh
fi

mkdir -p ../cmake_build_dir/ecflow/$mode_arg
cd ../cmake_build_dir/ecflow/$mode_arg

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

cmake ../../../ecflow -DCMAKE_MODULE_PATH=$WK/../ecbuild/cmake \
            -DCMAKE_BUILD_TYPE=$cmake_build_type \
            -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/$release.$major.$minor \
            -DCMAKE_PYTHON_INSTALL_TYPE=local \
            -DENABLE_WARNINGS=ON \
            -DCMAKE_CXX_FLAGS="-Wno-unused-local-typedefs" \
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
if [[ $test_arg = test || $test_safe_arg = test_safe ]] ; then
	ctest -R ^u_
	ctest -R c_
	ctest -R py_u
	ctest -R s_client
fi
if [[ $test_arg = test ]] ; then
	ctest -R s_test
	ctest -R py_s
fi

# =============================================================================================
if [[ $package_source_arg = package_source ]] ; then
	make package_source
	
	if [[ $copy_tarball_arg = copy_tarball ]] ; then
		rm -rf /tmp/$USER/tmp
		mkdir -p /tmp/$USER/tmp
		cp ecFlow-$release.$major.$minor-Source.tar.gz  /tmp/$USER/tmp/.
		cd /tmp/$USER/tmp/
		tar -zxf ecFlow-$release.$major.$minor-Source.tar.gz
	fi
	
	cp ecFlow-$release.$major.$minor-Source.tar.gz $SCRATCH/.
fi


# =========================================================================================
# NOTES:
# Boost:  
#  By default it looks for environment variable BOOST_ROOT, if not it can specified on the command line. i.e
#  -DBOOST_ROOT=/var/tmp/ma0/boost/boost_1_53_0

# ============================================================================================
# Python:
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
#    local : this will install to $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory
