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
# ========================================================
# Usage instructions:
#
# cd $WK
# mkdir ecbuild
# cd ecbuild
# Build debug and release in same tree
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
#         -DCMAKE_INSTALL_PREFIX=/usr/local/apps/ecflow/$(cat ../../VERSION.cmake | awk '{print $3}'|sed 's/["]//g')
#
#         -DCMAKE_INSTALL_PREFIX=/var/tmp/ma0/cmake/ecflow/4.0.1
#
#############################################################

cmake_minimum_required( VERSION 2.6.4 FATAL_ERROR )

project( ecflow CXX )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild/cmake")

include( ecbuild_system )

ecbuild_requires_macro_version( 1.2 )

###############################################################################
# local project

ecbuild_declare_project()

###############################################################################
# some variables of this project

option( ENABLE_PYTHON         "enable python interface"   ON  )

###############################################################################
# find packages

# Python ----------------------------------------------------------------------
if(ENABLE_PYTHON)
   ecbuild_find_python( VERSION 2.7 REQUIRED )
else() 
   ecbuild_find_python( VERSION 2.7 )
endif()
message( STATUS "PYTHON_EXECUTABLE    : ${PYTHON_EXECUTABLE}")
message( STATUS "PYTHON_INCLUDE_DIRS  : ${PYTHON_INCLUDE_DIRS}")
message( STATUS "PYTHON_LIBRARIES     : ${PYTHON_LIBRARIES}")
 

# Boost ----------------------------------------------------------------------

# If this is enable then can not find boost libs using env BOOST_ROOT ?
#ecbuild_add_extra_search_paths( boost )

# For local boost_root I do not set Boost_USE_STATIC_LIBS, but it still links boost libs statically
# for external boost root its need to be explicitly enabled ?
#set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_SYSTEM_PATHS        ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
#set(Boost_DEBUG                 ON)

find_package( Boost 1.53.0 COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time )
#find_package(Boost 1.53.0 EXACT REQUIRED COMPONENTS python)

message( STATUS "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )

list( APPEND Ecflow_Boost_LIBS 
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
)

list( APPEND Ecflow_Test_LIBS 
      ${Ecflow_Boost_LIBS} 
      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} 
      ${Boost_TEST_EXEC_MONITOR_LIBRARY} )

##############################################################################
# debug
if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )
    message( "INFO: DEBUG BUILD" )
    # for debug type builds, turn on verbose makefiles
    # set(CMAKE_VERBOSE_MAKEFILE ON)

    # Tell other CMake files that we're doing a debug build
    set( DEBUG_BUILD 1 )

    # Tell C/C++ that we're doing a debug build
    add_definitions( -DDEBUG )
endif()


###############################################################################
# build source code

# compile with fPIC on linux, need for ecflow.so *install*
# This could be switched off for, Server,/View, since that is not used in ecflow.so
cmake_add_cxx_flags("-fPIC")

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

# configure_file( cmake/ecbase_config.h.in  ${CMAKE_BINARY_DIR}/ecbase_config.h )


# =========================================================================================
# install 
# =========================================================================================
# install scripts
install (FILES  
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
install (FILES  
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/client_options.docx  
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/user_manual.docx    
            ${CMAKE_SOURCE_DIR}/Doc/user-manual/user_manual.pdf    
         DESTINATION doc/ecflow
         PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
        )
         
############################################################################################
# final

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )

# print the summary of the configuration
ecbuild_print_summary()
