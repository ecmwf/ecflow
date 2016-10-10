#!/bin/ksh
# ==================================================================
# Error handling
set -e # stop the shell on first error
set -u # fail when using an undefined variable

# ensure correct permission for installation
umask 0022

# ====================================================================
show_error_and_exit() {
   echo "cmake.sh expects at least one argument"
   echo " cmake.sh debug || release [clang] [san] [make] [verbose] [test] [stest] [no_gui] [package_source] [debug]"
   echo "  "
   echo "   make           - run make after cmake"
   echo "   test           - run all the tests"
   echo "   test_safe      - only run deterministic tests"
   echo "   ctest          - all ctest -R <test> -V"
   echo "   san            - is short for clang thread sanitiser"
   echo "   no_gui         - Don't build the gui"
   echo "   ssl            - build using openssl"
   echo "   log            - enable debug output"
   echo "   package_source - produces ecFlow-<version>-Source.tar.gz file, for users"
   echo "                    copies the tar file to $SCRATCH"
   echo "   copy_tarball   - copies ecFlow-<version>-Source.tar.gz to /tmp/$USER/tmp/. and untars file"
   exit 1
}

copy_tarball_arg=
package_source_arg=
make_arg=
make_only_arg=
test_arg=
test_safe_arg=
clang_arg=
intel_arg=
clang_sanitiser_arg=
mode_arg=release
verbose_arg=
ctest_arg=
clean_arg=
no_gui_arg=
python3_arg=
ssl_arg=
log_arg=
while [[ "$#" != 0 ]] ; do   
   if [[ "$1" = debug || "$1" = release ]] ; then
      mode_arg=$1
   elif  [[ "$1" = make_only ]] ; then
      make_only_arg=make
      shift
      while [[ "$#" != 0 ]] ; do
         make_only_arg="$make_only_arg $1"
         shift
      done
      break
   elif  [[ "$1" = make ]] ; then
      make_arg=$1
      shift
      while [[ "$#" != 0 ]] ; do
         make_arg="$make_arg $1"
         shift
      done
      break
   elif [[ "$1" = no_gui ]] ; then no_gui_arg=$1 ;
   elif [[ "$1" = ssl ]]   ; then ssl_arg=$1 ;
   elif [[ "$1" = log ]]   ; then log_arg=$1 ;
   elif [[ "$1" = clang ]] ; then clang_arg=$1 ;
   elif [[ "$1" = intel ]] ; then intel_arg=$1 ;
   elif [[ "$1" = clean ]] ; then clean_arg=$1 ;
   elif [[ "$1" = san ]]   ; then clang_sanitiser_arg=$1 ;
   elif [[ "$1" = package_source ]] ; then package_source_arg=$1 ;
   elif [[ "$1" = copy_tarball ]] ; then copy_tarball_arg=$1 ;
   elif [[ "$1" = test ]] ;  then test_arg=$1 ;
   elif [[ "$1" = test_safe ]] ; then test_safe_arg=$1 ;
   elif [[ "$1" = python3 ]] ;   then python3_arg=$1 ;
   elif [[ "$1" = ctest ]] ; then  
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

echo "copy_tarball_arg=$copy_tarball_arg"
echo "package_source_arg=$package_source_arg"
echo "make_arg=$make_arg"
echo "test_arg=$test_arg"
echo "test_safe_arg=$test_safe_arg"
echo "clang_arg=$clang_arg"
echo "clang_sanitiser_arg=$clang_sanitiser_arg"
echo "mode_arg=$mode_arg"
echo "verbose_arg=$verbose_arg"
echo "python3_arg=$python3_arg"
echo "no_gui_arg=$no_gui_arg"
set -x # echo script lines as they are executed

# ==================== modules ================================================
# To load module automatically requires Korn shell, system start scripts

module load cmake/3.3.2
module load ecbuild/2.4.0
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
if [[ "$ARCH" = cray ]] ; then
    cmake_extra_options="$cmake_extra_options -DENABLE_UI=OFF"
    
    if [[ $intel_arg = intel ]] ; then
        module swap PrgEnv-cray PrgEnv-intel
    else
    	module swap PrgEnv-cray PrgEnv-gnu
    fi
    module load boost/1.53.0
    export CRAY_ADD_RPATH=no
    export ECFLOW_CRAY_BATCH=1
fi

if [[ "$python3_arg" = python3 ]] ; then
    # Need to wait for ecbuild to fix print error, meanwhile use local ecbuild to test python3
    cmake_extra_options="$cmake_extra_options -DPYTHON_EXECUTABLE=/usr/local/apps/python3/3.5.1-01/bin/python3.5"
fi

# boost
if [[ $OS_VERSION = leap42 ]] ; then
    module load boost/1.53.0
fi

# ====================================================================================  
cmake_build_type=
if [[ $mode_arg = debug ]] ; then
    cmake_build_type=Debug
else
    cmake_build_type=Release
fi

# ====================================================================================
# Use for local install
release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')

# ====================================================================================
# clean up source before packaging, do this after deleting ecbuild
if [[ $package_source_arg = package_source ]] ; then
	source build_scripts/clean.sh
fi

# =======================================================================================
# Change directory
source_dir=$(pwd)
workspace=$(pwd)/..

if [[ $clean_arg = clean ]] ; then
	rm -rf ../bdir/$mode_arg/ecflow
fi

mkdir -p ../bdir/$mode_arg/ecflow
cd ../bdir/$mode_arg/ecflow


# =============================================================================================
if [[ $test_arg = test || $test_safe_arg = test_safe ]] ; then
	ctest -R ^u_
	ctest -R c_
	ctest -R py_u
	ctest -R s_client
	if [[  $test_safe_arg = test_safe ]] ; then
	   exit 0
	fi
fi
if [[ $test_arg = test ]] ; then
	ctest -R s_test
	ctest -R py_s
	exit 0
fi

# ctest 
if [[ "$ctest_arg" != "" ]] ; then
	$ctest_arg
	exit 0
fi

if [[ "$make_only_arg" != "" ]] ; then
	$make_only_arg
	exit 0
fi

# ====================================================================================
#
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
# -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#   *AND* for testing python install to local directory
#
# GNU 4.8+ -Wno-unused-local-typedefs   -> get round warning in boost headers
# GNU 6.1  -Wno-deprecated-declarations -> auto_ptr deprecated warning, mostly in boost headers  

ssl_options=
if [[ $ssl_arg = ssl ]] ; then
    ssl_options="-DENABLE_SSL=ON"
fi

log_options=
if [[ $log_arg = log ]] ; then
    log_options="-DECBUILD_LOG_LEVEL=DEBUG"
fi

gui_options=
if [[ $no_gui_arg = no_gui ]] ; then
    gui_options="-DENABLE_GUI=OFF -DENABLE_UI=OFF -DENABLE_ALL_TESTS=ON"
fi

if [[ $package_source_arg = package_source ]] ; then
    # for packaging we build GUI by default, and do not run all tests
    gui_options=  
fi

#$workspace/ecbuild/bin/ecbuild $source_dir \
ecbuild $source_dir \
            -DCMAKE_BUILD_TYPE=$cmake_build_type \
            -DCMAKE_INSTALL_PREFIX=/var/tmp/$USER/install/cmake/ecflow/$release.$major.$minor \
            -DENABLE_WARNINGS=ON \
            -DCMAKE_CXX_FLAGS="-Wno-unused-local-typedefs" \
            -DCMAKE_PYTHON_INSTALL_TYPE=local \
            -DCMAKE_PREFIX_PATH="/usr/local/apps/qt/5.5.0/5.5/gcc_64/" \
            -DENABLE_STATIC_BOOST_LIBS=ON \
            ${cmake_extra_options} \
            ${gui_options} \
            ${ssl_options} \
            ${log_options}
            #-DENABLE_ALL_TESTS=ON
            #-DENABLE_GUI=ON       -DENABLE_UI=ON                    
            #-DENABLE_SERVER=OFF \
            #-DCMAKE_PYTHON_INSTALL_PREFIX=/var/tmp/$USER/install/python/ecflow/$release.$major.$minor \
            #-DCMAKE_CXX_FLAGS="'-Wno-unused-local-typedefs -Wno-deprecated'"
        
# =============================================================================================
if [[ "$make_arg" != "" ]] ; then
	$make_arg
	exit 0
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
#  -DBOOST_ROOT=/var/tmp/$USER/boost/boost_1_53_0

# ============================================================================================
# Python:
# -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup ]
#    local : this will install to $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup : experimental only, python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory
