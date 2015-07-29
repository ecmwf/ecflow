############################################################################################
# cmake options:
#
#       -DCMAKE_BUILD_TYPE=Debug|RelWithDebInfo|Release|Production
#       -DCMAKE_INSTALL_PREFIX=/path/to/install
#
#       -DCMAKE_MODULE_PATH=/path/to/ecbuild/cmake
#
#       -DCMAKE_C_COMPILER=gcc
#       -DCMAKE_CXX_COMPILER=g++
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
########################################################################

cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

# Change the default if NO CMAKE_BUILD_TYPE specified
if (NOT DEFINED CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Configuration type" FORCE)
endif()

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
# The ecflow version config is done in ACore directory
# =========================================================================================

message( STATUS "CMAKE_INSTALL_PREFIX : ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "ECFLOW_RELEASE       : ${ECFLOW_RELEASE}" )
message( STATUS "ECFLOW_MAJOR         : ${ECFLOW_MAJOR}" )
message( STATUS "ECFLOW_MINOR         : ${ECFLOW_MINOR}" )
message( STATUS "ECFLOW_VERSION       : ${ECFLOW_VERSION_STR}" )

# =========================================================================================
# Options , be aware of caching, when modifying on the command line. 
# Ideally start fresh or remove cache CmakeCache.txt in build dir
# =========================================================================================
option( ENABLE_PYTHON         "enable python interface"   ON  )
option( ENABLE_GUI            "Build ecflowview"          ON  )

message( STATUS "ENABLE_PYTHON         : ${ENABLE_PYTHON}" )
message( STATUS "ENABLE_GUI            : ${ENABLE_GUI}" )

# ======================================================================================
# GUI requires X11 and Motif
# ======================================================================================

find_package( X11 )

#if ( X11_FOUND )
#	debug_var( X11_INCLUDE_DIR )
#	debug_var( X11_LIBRARIES )  
#	debug_var( X11_Xt_INCLUDE_PATH )
#	debug_var( X11_Xt_LIB )
#	debug_var( X11_Xpm_INCLUDE_PATH )
#	debug_var( X11_Xpm_LIB )
#endif()

find_package( Motif )

#if ( MOTIF_FOUND )
#	debug_var( MOTIF_INCLUDE_DIR )
#	debug_var( MOTIF_LIBRARIES )  
#endif()
 
 
# =========================================================================================
# Boost
# =========================================================================================

ecbuild_add_extra_search_paths( boost ) # also respects BOOST_ROOT

# Ecflow test require statics libs, otherwise get double free, error.
# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
# See: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_SYSTEM_PATHS        ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
#set(Boost_DEBUG                  ON)

find_package( Boost 1.53.0 REQUIRED COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time )

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


#message( STATUS "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )


# =========================================================================================
# debug
# =========================================================================================

if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )

	message( STATUS "INFO: DEBUG BUILD" )

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

if (ENABLE_PYTHON)
    ecbuild_find_python( VERSION 2.7 REQUIRED )
    #debug_var(PYTHON_LIBRARIES)
    #debug_var(PYTHON_INCLUDE_DIR)
	add_subdirectory( Pyext )
endif()

if( ENABLE_GUI AND X11_FOUND AND MOTIF_FOUND )
   add_subdirectory( view )
endif()

# =========================================================================================
# tar ball, by default ecbuild will tar everything apart from hard wired directory called 'build' 
#   hence we can tell it, what directories not to pack
# =========================================================================================

ecbuild_dont_pack( DIRS 
                   	ACore/doc ANattr/doc ANode/doc Client/doc Pyext/doc Server/doc
                   	ecbuild
                    SCRATCH 
                    build_scripts/nightly build_scripts/test_bench
                    Doc/func_spec Doc/misc Doc/New_viewer Doc/newsletter Doc/online Doc/presentations Doc/tac
)

# =========================================================================================
# install scripts
# =========================================================================================

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
