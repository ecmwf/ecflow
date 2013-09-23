############################################################################################
# cmake
#   options:
#       -DCMAKE_BUILD_TYPE=Debug|RelWithDebInfo|Release|Production
#       -DCMAKE_INSTALL_PREFIX=/path/to/install
#
#       -DCMAKE_C_COMPILER=gcc
#       -DCMAKE_C_COMPILER=g++
#
#       -DCMAKE_PREFIX_PATH=/path/to/jasper:/path/to/any/package/out/of/place

cmake_minimum_required( VERSION 2.6.4 FATAL_ERROR )

project( ecflow CXX )

set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )

include( ecbuild_system )

###############################################################################
# local project

ecbuild_declare_project()

###############################################################################
# find packages

ecbuild_find_python( VERSION 2.7 )

# set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
# set(Boost_NO_SYSTEM_PATHS        ON)
find_package( Boost 1.47.0 COMPONENTS python serialization system thread unit_test_framework test_exec_monitor filesystem program_options date_time )

list( APPEND Ecflow_Boost_LIBS 
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
)

list( APPEND Ecflow_Test_LIBS ${Ecflow_Boost_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} )

if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )
    message( "INFO: DEBUG BUILD" )
    # for debug type builds, turn on verbose makefiles
    # SET(CMAKE_VERBOSE_MAKEFILE ON)

    # Tell other CMake files that we're doing a debug build
    SET( DEBUG_BUILD 1 )

    # Tell C/C++ that we're doing a debug build
    ADD_DEFINITIONS( -DDEBUG )
endif()

add_subdirectory( ACore )
add_subdirectory( ANattr )
add_subdirectory( ANode )
add_subdirectory( AParser )
add_subdirectory( Base )
add_subdirectory( Client )
add_subdirectory( CSim )
add_subdirectory( Server )
add_subdirectory( Test )
add_subdirectory( TestEcfSms )
add_subdirectory( Pyext )

add_subdirectory( view )

# configure_file( cmake/ecbase_config.h.in  ${CMAKE_BINARY_DIR}/ecbase_config.h )

############################################################################################
# finalize

ecbuild_install_project( NAME ecFlow )

ecbuild_print_summary()
