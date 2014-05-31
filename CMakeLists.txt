############################################################################################
# cmake options:
#
#       -DCMAKE_BUILD_TYPE=Debug|RelWithDebInfo|Release|Production
#       -DCMAKE_INSTALL_PREFIX=/path/to/install
#
#       -DCMAKE_MODULE_PATH=/path/to/ecbuild/cmake
#
#       -DCMAKE_C_COMPILER=gcc
#       -DCMAKE_C_COMPILER=g++
#
#       -DCMAKE_PREFIX_PATH=/path/to/jasper:/path/to/any/package/out/of/place
#       -DBUILD_SHARED_LIBS=OFF
# =========================================================================================
# Usage instructions:
#
# cd $WK
# release=$(cat VERSION.cmake | grep 'set( ECFLOW_RELEASE' | awk '{print $3}'| sed 's/["]//g')
# major=$(cat VERSION.cmake   | grep 'set( ECFLOW_MAJOR'   | awk '{print $3}'| sed 's/["]//g')
# minor=$(cat VERSION.cmake   | grep 'set( ECFLOW_MINOR'   | awk '{print $3}'| sed 's/["]//g')
#
# mkdir ecbuild
# cd ecbuild
# mkdir debug
# mkdir release
# cd debug
#
#/usr/local/apps/cmake/current/bin/cmake ../../  \
#         -DCMAKE_BUILD_TYPE=Debug \
#         -DCMAKE_MODULE_PATH=/usr/local/apps/ecbuild/current/share/ecbuild/cmake \
#         -DPYTHON_LIBRARY=/usr/local/apps/python/current/lib/libpython2.7.a \
#         -DPYTHON_INCLUDE_DIR=/usr/local/apps/python/current/include/python2.7 \
#         -DPYTHON_EXECUTABLE=/usr/local/apps/python/current/bin/python2.7 \
#         -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow/$release.$major.$minor
#
#         -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/4.0.1
#
#############################################################

cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

project( ecflow CXX )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild/cmake")
# message( STATUS "CMAKE_MODULE_PATH    : ${CMAKE_MODULE_PATH}")

include( ecbuild_system )

ecbuild_requires_macro_version( 1.3 )

###############################################################################
# local project

ecbuild_declare_project()

# =========================================================================================
# VERSION  
# Get the ecflow version specified in 'VERSION.cmake'. This is only accessible after ecbuild_declare_project()
# The ecflow version cofig is done in ACore directory
# =========================================================================================

message( STATUS "CMAKE_INSTALL_PREFIX : ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "ECFLOW_RELEASE       : ${ECFLOW_RELEASE}" )
message( STATUS "ECFLOW_MAJOR         : ${ECFLOW_MAJOR}" )
message( STATUS "ECFLOW_MINOR         : ${ECFLOW_MINOR}" )
message( STATUS "ECFLOW_VERSION       : ${ECFLOW_VERSION_STR}" )

# =========================================================================================
# Python  
# =========================================================================================
# some variables of this project

option( ENABLE_PYTHON         "enable python interface"   ON  )

if(ENABLE_PYTHON)
   ecbuild_find_python( VERSION 2.7 REQUIRED )
else() 
   ecbuild_find_python( VERSION 2.7 )
endif()

# =========================================================================================
# Boost  
# =========================================================================================

ecbuild_add_extra_search_paths( boost ) # also respects BOOST_ROOT

# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_SYSTEM_PATHS        ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
#set(Boost_DEBUG                 ON)

find_package( Boost 1.53.0 COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time )
#find_package(Boost 1.53.0 EXACT REQUIRED COMPONENTS python)
#
# Available boost lib should be referenced as:
#
#    ${Boost_SYSTEM_LIBRARY}
#    ${Boost_SERIALIZATION_LIBRARY}
#    ${Boost_THREAD_LIBRARY}
#    ${Boost_FILESYSTEM_LIBRARY}
#    ${Boost_PROGRAM_OPTIONS_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY}
#
#    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} 
#    ${Boost_TEST_EXEC_MONITOR_LIBRARY} 

message( STATUS "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )

#list( APPEND Ecflow_Boost_LIBS
#    ${Boost_SERIALIZATION_LIBRARY}
#    ${Boost_SYSTEM_LIBRARY}
#    ${Boost_THREAD_LIBRARY}
#    ${Boost_FILESYSTEM_LIBRARY}
#    ${Boost_PROGRAM_OPTIONS_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY}
#)

# =========================================================================================
# debug
# =========================================================================================
if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )

	message( STATUS "INFO: DEBUG BUILD" )

    # for debug type builds, turn on verbose makefiles
    # set(CMAKE_VERBOSE_MAKEFILE ON)

    # Tell other CMake files that we're doing a debug build
	# set( DEBUG_BUILD 1 ) -- not used anymore

    # Tell C/C++ that we're doing a debug build
    add_definitions( -DDEBUG )

endif()


# =========================================================================================
# build source code
# =========================================================================================

add_subdirectory( ACore )
add_subdirectory( ANattr )
add_subdirectory( ANode )
add_subdirectory( AParser )
add_subdirectory( Base )
add_subdirectory( Client )
add_subdirectory( CSim )
add_subdirectory( Server )
add_subdirectory( Test )
add_subdirectory( Pyext )
add_subdirectory( view )

# Directories not need in the distribution tar ball!
ecbuild_add_resources( TARGET build_nightly DONT_PACK_DIRS build/nightly)
ecbuild_add_resources( TARGET SCRATCH DONT_PACK_DIRS SCRATCH)
ecbuild_add_resources( TARGET Doc DONT_PACK_DIRS Doc/func_spec Doc/misc Doc/newsletter Doc/online Doc/presentations Doc/tac)

# =========================================================================================
# install 
# =========================================================================================
# install scripts
install( FILES
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.pl
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_start.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_stop.sh
            ${CMAKE_SOURCE_DIR}/tools/noconnect.sh
            ${CMAKE_SOURCE_DIR}/Pyext/migrate/ecflow_migrate.py
         DESTINATION bin
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
        )

# install documentation         
install( FILES
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/client_options.docx  
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/user_manual.docx    
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/user_manual.pdf    
         DESTINATION doc/ecflow
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
        )
        
# =========================================================================================
# final
# =========================================================================================

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )

# print the summary of the configuration
ecbuild_print_summary()
