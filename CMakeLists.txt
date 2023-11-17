############################################################################################
# cmake options:
#
#       -DCMAKE_BUILD_TYPE=Debug|RelWithDebInfo|Release
#       -DCMAKE_INSTALL_PREFIX=/path/to/install
#
#       -DCMAKE_MODULE_PATH=/path/to/ecbuild/cmake
#
#       -DCMAKE_C_COMPILER=gcc
#       -DCMAKE_CXX_COMPILER=g++
#
#       -DCMAKE_PREFIX_PATH=/path/to/jasper:/path/to/any/package/out/of/place
#       -DBUILD_SHARED_LIBS=OFF
##############################################################################

# need 3.12.0 as this has findBoost fix for the latest boost version 1.67
cmake_minimum_required( VERSION 3.12.0 FATAL_ERROR )

# ===========================================================================
# Change the default if NO CMAKE_BUILD_TYPE specified
if (NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Configuration type" FORCE)
endif()

find_package( ecbuild 3.4 REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild ) # Before project()  

# this will generate variables, see ACore/ecflow_version.h.in
project( ecflow LANGUAGES CXX VERSION 5.11.3 )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include( ecbuild_system NO_POLICY_SCOPE )

ecbuild_requires_macro_version( 1.6 )
include(CompilerOptions)

###############################################################################
# local project

ecbuild_declare_project()

ecbuild_info( "CMAKE_MODULE_PATH        : ${CMAKE_MODULE_PATH}")
ecbuild_info( "CMAKE_INSTALL_PREFIX     : ${CMAKE_INSTALL_PREFIX}" )
ecbuild_info( "ecflow_BINARY_DIR        : ${ecflow_BINARY_DIR}" )
ecbuild_info( "ecflow_SOURCE_DIR        : ${ecflow_SOURCE_DIR}" )


# =========================================================================================
# Options , be aware of caching, when modifying on the command line. 
# Ideally start fresh or remove cache CmakeCache.txt in build dir
# =========================================================================================
option( ENABLE_SERVER              "Build the server, can be switched off if just building UI" ON  )
option( ENABLE_PYTHON              "enable python interface"         ON  )
option( ENABLE_UI                  "Build ecFlowUI"                  ON  ) 
option( ENABLE_STATIC_BOOST_LIBS   "Link with boost libs statically" ON  )
option( ENABLE_ALL_TESTS           "This includes performance/migration/regression tests" OFF )
option( ENABLE_UI_BACKTRACE        "Print a UI debug backtrace"         OFF ) 
option( ENABLE_UI_USAGE_LOG        "Enable UI usage logging"            OFF )
option( ENABLE_SSL                 "Enable SSL encrypted communication" ON )
option( ENABLE_PYTHON_PTR_REGISTER "Some compilers/boost versions do not register shared ptr automatically" OFF  )
option( ENABLE_PYTHON_UNDEF_LOOKUP "Some boost/python versions are too closely linked" OFF  )
option( ENABLE_HTTP                "Enable HTTP server (experimental)" ON  )
option( ENABLE_UDP                 "Enable UDP server (experimental)" ON )
option( ENABLE_DOCS                "Enable Documentation" OFF )


# =========================================================================================
# Dependency: Qt
# =========================================================================================
# Qt for ecFlowUI.
# Algorithm: we test for Qt6 - if it's there, then use it; otherwise look for Qt5.
#            if we don't find that, then we cannot build ecFlowUI.
# -----------------------------------------------------------------------------------------
if(ENABLE_UI)

  ecbuild_info("")
  ecbuild_info("------------------------------ Qt start ---------------------------")
  ecbuild_info("ecFlowUI is enabled - searching for Qt6/Qt5")
  ecbuild_info("To use a self-built Qt installation, try setting CMAKE_PREFIX_PATH")

  find_package(Qt6Widgets)
  if( Qt6Widgets_FOUND )
    find_package(Qt6Core5Compat  REQUIRED)
    find_package(Qt6Gui          REQUIRED)
    find_package(Qt6Network      REQUIRED)
    find_package(Qt6Svg          REQUIRED)
    find_package(Qt6Charts)

    ecbuild_debug("Qt6 version ${Qt6Widgets_VERSION_STRING} was found")
    set(ECFLOW_QT_INCLUDE_DIR ${Qt6Core5Compat_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS} ${Qt6Svg_INCLUDE_DIRS})
    set(ECFLOW_QT_LIBRARIES  ${Qt6Core5Compat_LIBRARIES} ${Qt6Widgets_LIBRARIES}    ${Qt6Gui_LIBRARIES}   ${Qt6Network_LIBRARIES} ${Qt6Svg_LIBRARIES})
    set(ECFLOW_QT6 1)
    add_definitions(-DECFLOW_QT6)

    if(Qt6Charts_FOUND)
      ecbuild_info("Qt6Charts was found - the server log viewer will be built")
      list(APPEND ECFLOW_QT_INCLUDE_DIR ${Qt6Charts_INCLUDE_DIRS})
      list(APPEND ECFLOW_QT_LIBRARIES  ${Qt6Charts_LIBRARIES})
      set(ECFLOW_LOGVIEW 1)
      add_definitions(-DECFLOW_LOGVIEW)
    else()
      ecbuild_info("Qt6Charts was not found - the server log viewer will not be built")
    endif()
  else()
    find_package(Qt5Widgets)
    if (Qt5Widgets_FOUND)
      find_package(Qt5Gui          REQUIRED)
      find_package(Qt5Network      REQUIRED)
      find_package(Qt5Svg          REQUIRED)
      find_package(Qt5Charts)

      ecbuild_info("Qt5 version ${Qt5Widgets_VERSION_STRING} was found")
      set(ECFLOW_QT_INCLUDE_DIR ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Svg_INCLUDE_DIRS})
      set(ECFLOW_QT_LIBRARIES   ${Qt5Widgets_LIBRARIES}    ${Qt5Gui_LIBRARIES}   ${Qt5Network_LIBRARIES} ${Qt5Svg_LIBRARIES})
      set(ECFLOW_QT5 1)
      add_definitions(-DECFLOW_QT5)

      if(Qt5Charts_FOUND)
        ecbuild_info("Qt5Charts was found - the server log viewer will be built")
        list(APPEND ECFLOW_QT_INCLUDE_DIR ${Qt5Charts_INCLUDE_DIRS})
        list(APPEND ECFLOW_QT_LIBRARIES  ${Qt5Charts_LIBRARIES})
        set(ECFLOW_LOGVIEW 1)
        add_definitions(-DECFLOW_LOGVIEW)
      else()
        ecbuild_info("Qt5Charts was not found - the server log viewer will not be built")
      endif()
    else()
      ecbuild_critical("Qt5 Widgets, Network and Gui not found - these are required for ecFlowUI; or use -DENABLE_UI=OFF")
    endif()
  endif()

  set(ECFLOW_QT 1)
  add_definitions(-DQT_NO_KEYWORDS) # We need to disable keywords because there is a problem in using Qt and boost together.
  ecbuild_info("------------------------------ Qt end----------------------------")
  ecbuild_info("")

endif()


# sanity check - cannot set ENABLE_UI_BACKTRACE if UI is OFF
if(ENABLE_UI_BACKTRACE AND (NOT ENABLE_UI))
  ecbuild_warn("Cannot ENABLE_UI_BACKTRACE if UI is not enabled")
  set(ENABLE_UI_BACKTRACE OFF)
endif()

# sanity check - cannot set UI_BACKTRACE_EMAIL_ADDRESS_FILE if UI and ENABLE_UI_BACKTRACE are OFF
if(UI_BACKTRACE_EMAIL_ADDRESS_FILE AND (NOT ENABLE_UI))
  ecbuild_warn("Cannot set UI_BACKTRACE_EMAIL_ADDRESS_FILE if UI is not enabled")
  set(UI_BACKTRACE_EMAIL_ADDRESS_FILE)
endif()

# sanity check - cannot set UI_LOG_FILE if ENABLE_UI_USAGE_LOG is OFF
if(UI_LOG_FILE AND (NOT ENABLE_UI_USAGE_LOG))
  ecbuild_warn("Cannot set UI_LOG_FILE if ENABLE_UI_USAGE_LOG is not enabled")
  set(UI_LOG_FILE)
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_FILE
if(ENABLE_UI_USAGE_LOG AND (NOT UI_LOG_FILE))
  ecbuild_error("If ENABLE_UI_USAGE_LOG is set, UI_LOG_FILE must also be set")
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_FILE
if(ENABLE_UI_USAGE_LOG AND (NOT LOGUI_LOG_FILE))
  ecbuild_error("If ENABLE_UI_USAGE_LOG is set, LOGUI_LOG_FILE must also be set")
endif()

# sanity check - if ENABLE_UI_USAGE_LOG is ON, we must also have UI_LOG_SITE_TAG
if(ENABLE_UI_USAGE_LOG AND (NOT UI_LOG_SITE_TAG))
  ecbuild_error("If ENABLE_UI_USAGE_LOG is set, UI_LOG_SITE_TAG must also be set")
endif()

# sanity check - cannot set UI_SYSTEM_SERVERS_LIST if UI IS OFF
if(UI_SYSTEM_SERVERS_LIST AND (NOT ENABLE_UI))
  ecbuild_warn("Cannot set UI_SYSTEM_SERVERS_LIST if UI is not enabled")
  set(UI_SYSTEM_SERVERS_LIST)
endif()

if(ENABLE_HTTP AND NOT ENABLE_SERVER)
  ecbuild_warn("ENABLE_SERVER is disabled, therefore HTTP_SERVER will also be disabled")
  set(ENABLE_HTTP OFF)
endif()

if(ENABLE_UDP AND NOT ENABLE_SERVER)
  ecbuild_warn("ENABLE_SERVER is disabled, therefore UDP_SERVER will also be disabled")
  set(ENABLE_UDP OFF)
endif()


ecbuild_info( "ENABLE_SERVER              : ${ENABLE_SERVER}" )
ecbuild_info( "ENABLE_PYTHON              : ${ENABLE_PYTHON}" )
ecbuild_info( "ENABLE_PYTHON_PTR_REGISTER : ${ENABLE_PYTHON_PTR_REGISTER}" )
ecbuild_info( "ENABLE_PYTHON_UNDEF_LOOKUP : ${ENABLE_PYTHON_UNDEF_LOOKUP}" )
ecbuild_info( "ENABLE_UI                  : ${ENABLE_UI}" )
ecbuild_info( "ENABLE_TESTS               : ${ENABLE_TESTS} *if* disabled no need for boost test libs" )
ecbuild_info( "ENABLE_ALL_TESTS           : ${ENABLE_ALL_TESTS}" )
ecbuild_info( "ENABLE_STATIC_BOOST_LIBS   : ${ENABLE_STATIC_BOOST_LIBS}" )
ecbuild_info( "ENABLE_SSL                 : ${ENABLE_SSL} *if* openssl libraries available" )
ecbuild_info( "ENABLE_HTTP                : ${ENABLE_HTTP}" )
ecbuild_info( "ENABLE_UDP                 : ${ENABLE_UDP}" )


if (ENABLE_UI)
  ecbuild_info( "ENABLE_UI_BACKTRACE        : ${ENABLE_UI_BACKTRACE}" )
  if (UI_BACKTRACE_EMAIL_ADDRESS_FILE)
    ecbuild_info( "UI_BACKTRACE_EMAIL_ADDRESS_FILE : ${UI_BACKTRACE_EMAIL_ADDRESS_FILE}" )
  endif()

  if(LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE)
    ecbuild_info( "LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE : ${LOGUI_BACKTRACE_EMAIL_ADDRESS_FILE}" )
  endif()

  if(UI_SYSTEM_SERVERS_LIST)
    ecbuild_info( "UI_SYSTEM_SERVERS_LIST : ${UI_SYSTEM_SERVERS_LIST}" )
  endif()

  ecbuild_info( "ENABLE_UI_USAGE_LOG        : ${ENABLE_UI_USAGE_LOG}" )
  if(ENABLE_UI_USAGE_LOG)
    ecbuild_info( "UI_LOG_FILE                : ${UI_LOG_FILE}" )
    ecbuild_info( "LOGUI_LOG_FILE             : ${LOGUI_LOG_FILE}" )
    ecbuild_info( "UI_LOG_SITE_TAG            : ${UI_LOG_SITE_TAG}" )
  endif()
endif()
ecbuild_info("")


# =========================================================================================
# Setup Project-wide dependencies
# =========================================================================================
include(Dependencies)

# =========================================================================================
# Dependency: Boost
# =========================================================================================
message( STATUS "====================================================================================================================" )
message( STATUS "BOOST" )
# this can help in some situations:
set (_CMAKE_PREFIX_PATH_BACKUP ${CMAKE_PREFIX_PATH}) # make a backup of CMAKE_PREFIX_PATH
foreach(BOOST_PATH_TO_SEARCH ${BOOST_PATH} ${BOOST_ROOT} $ENV{BOOST_ROOT})
    ecbuild_debug("adding: ${BOOST_PATH_TO_SEARCH} to CMAKE_PREFIX_PATH")
    list(APPEND CMAKE_PREFIX_PATH ${BOOST_PATH_TO_SEARCH})
    list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endforeach()

# Ecflow test require statics libs for boost < 1.59, otherwise get double free error.
# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
# See: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html
if ( ENABLE_STATIC_BOOST_LIBS )
    set(Boost_USE_STATIC_LIBS ON)
    ecbuild_info( "Using STATIC boost libraries : ${BOOST_TEST_DYN_LINK}" )
else()
    set(BOOST_TEST_DYN_LINK "BOOST_TEST_DYN_LINK")
    ecbuild_info( "Using SHARED boost libraries setting : ${BOOST_TEST_DYN_LINK}" )
endif()

# By default cmake assume boost was built with cmake. Hence looks for package config file BoostConfig.cmake/boost-config.cmake
# This seems to mess up the search for boost python libs.
# if Boost_NO_BOOST_CMAKE is not set, we seem to mess up in finding right python libs(python3 and python2), 
# for boost 1.71 and cmake 3.15 or greater
set(Boost_NO_BOOST_CMAKE ON) 

set(Boost_USE_MULTITHREADED  ON)
set(Boost_NO_SYSTEM_PATHS    ON)
set(Boost_DETAILED_FAILURE_MSG ON)
set(Boost_ARCHITECTURE       "-x64") # from boost 1.69 layout=tagged adds libboost_system-mt-x64.a, https://gitlab.kitware.com/cmake/cmake/issues/18908
#set(Boost_DEBUG                ON) # Set to ON to enable debug output from FindBoost

set(ECFLOW_BOOST_VERSION "1.66.0")

# need upfront to get a hold of Boost_VERSION_STRING
find_package( Boost ${ECFLOW_BOOST_VERSION} ) 

# HAVE_TESTS is defined if ecbuild ENABLE_TESTS is set, (by default this is set)
if(HAVE_TESTS)
  list(APPEND _boost_testlibs unit_test_framework test_exec_monitor )
endif()

# Need timer for boost::time::cpu_timer this needs boost:chrono, which need librt These are used in the test only
if ( Boost_VERSION_STRING VERSION_LESS "1.69.0" )
  find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS system timer chrono ${_boost_testlibs} filesystem program_options date_time )
else()
  # for boost version 1.69 or greater Boost.System is now header-only.
  find_package( Boost ${ECFLOW_BOOST_VERSION} REQUIRED COMPONENTS        timer chrono ${_boost_testlibs} filesystem program_options date_time )
endif()
ecbuild_info( "Boost_LIBRARIES     : ${Boost_LIBRARIES}" )


# =======================================================================================
# ??
# =======================================================================================
if (NOT "${CMAKE_PREFIX_PATH}" EQUAL "${_CMAKE_PREFIX_PATH_BACKUP}")
  set (CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH_BACKUP}) # restore CMAKE_PREFIX_PATH
  ecbuild_debug("Resetting CMAKE_PREFIX_PATH to ${CMAKE_PREFIX_PATH}")
endif()


# See: https://stackoverflow.com/questions/13653361/another-undefined-reference-error-when-linking-boost-libraries
message( STATUS "====================================================================================================================" )
message( STATUS "LIB RT  needed by some test, boost_timer->boost_chrono-> -lrt" )
find_library(LIBRT rt)                                                                                                                                                                                                                                                                                        
if(LIBRT)
  message( STATUS "LIB RT FOUND" )
else()
  # set to empty string to avoid lots of conditional around ecbuild_add_test
  message( STATUS "LIB RT NOTFOUND -> LIBRT set to empty string for ecbuild_add_test" )
  set(LIBRT "")
endif()

message( STATUS "====================================================================================================================" )
message( STATUS "SSL" )
if (ENABLE_SSL)
  find_package(OpenSSL REQUIRED)
  if (OPENSSL_FOUND)
    include_directories( ${OPENSSL_INCLUDE_DIR} )
    add_definitions( -DECF_OPENSSL )
  else()
    ecbuild_warn("Can *not* find openssl libraries. ecflow will build without ssl support")
  endif()
endif()

# =========================================================================================
# debug
# =========================================================================================

if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )

  ecbuild_info( "INFO: DEBUG BUILD" )

  # Tell C/C++ that we're doing a debug build
  add_definitions( -DDEBUG )

endif()

# =========================================================================================
# CLANG: /usr/local/include/boost/type_traits/is_base_and_derived.hpp:226:42: 
#           fatal error: recursive template instantiation exceeded maximum depth of 256
# =========================================================================================
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()

# =========================================================================================
# enable clang-format
# =========================================================================================
find_package(ClangFormat)

# =========================================================================================
# build source code
# =========================================================================================
add_subdirectory( ACore )
add_subdirectory( ANattr )
add_subdirectory( ANode )
add_subdirectory( Base )
add_subdirectory( CSim )
add_subdirectory( Client )

if (ENABLE_SERVER)
  add_subdirectory( Server )
  add_subdirectory( tools )
  add_subdirectory( Test )
endif()

if (ENABLE_PYTHON)
  if ( ENABLE_PYTHON_PTR_REGISTER )
    add_definitions( -DECF_ENABLE_PYTHON_PTR_REGISTER )
  endif()
  add_subdirectory( Pyext )
endif()

if (ENABLE_UI)
  add_subdirectory( Viewer )
  add_subdirectory( share )
endif()

if (ENABLE_HTTP)
  add_subdirectory( Http )
endif()

if (ENABLE_UDP)
  add_subdirectory( Udp )
endif()

# =========================================================================================
# DOXYGEN to use: make doxygen  -> ${CMAKE_CURRENT_BINARY_DIR}/Doc/doxygen/html/index.html
# =========================================================================================
find_package(Doxygen)
if (DOXYGEN_FOUND)
  ecbuild_info( "DOXYGEN_FOUND   - use 'make doxygen' first" )
   
  # exclude some dirs not related to documentation
  set( DOXYGEN_EXCLUDE_PATTERNS */bin/* */bdir/* */Debug/*  */test/*  */Doc/* */doc/* */samples/* SCRATCH tools build_scripts cereal )
  
  set( DOXYGEN_SOURCE_BROWSER YES)
  set( DOXYGEN_EXTRACT_PRIVATE YES)
  set( DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Doc/doxygen")
    
  # this target will only be built if specifically asked to.
  # run "make doxygen" to create the doxygen documentation
  doxygen_add_docs(
    doxygen
    ${PROJECT_SOURCE_DIR}
    COMMENT "Generate documentation for ecFlow"
  )
   
  # Add an install target to install the docs, *IF* the use has run 'make doxygen'
  if (EXISTS ${DOXYGEN_OUTPUT_DIRECTORY})
    install(DIRECTORY ${DOXYGEN_OUTPUT_DIRECTORY} DESTINATION ${CMAKE_INSTALL_DOCDIR})
  endif()
else ()
  ecbuild_info("Doxygen need to be installed to generate the doxygen documentation")
endif()

# =========================================================================================
# tar ball, by default ecbuild will tar everything apart from hard wired directory called 'build' 
# hence we can tell it, what directories not to pack
# =========================================================================================
ecbuild_dont_pack(
  DIRS
    .settings
    bamboo
    ACore/doc
    ANattr/doc
    ANode/doc
    Client/doc
    CSim/doc
    Pyext/doc
    Server/doc
    ecbuild
    SCRATCH
    CUSTOMER
    build_scripts/nightly
    build_scripts/test_bench
    Debug
    bdir
    bdir_xcode
    bin
    Doc/misc
    Doc/sphinx-examples
)

# ignore eclipse files
ecbuild_dont_pack(
  FILES
    .project
    .cproject
    .pydevproject
    Pyext/.pydevproject
    Pyext/samples/test.py
    Pyext/samples/confluence_add_attachment.py
    build_scripts/.pydevproject
)

# =========================================================================================
# Documentation
# =========================================================================================
if (ENABLE_DOCS)
  add_subdirectory(docs)
endif()

# =========================================================================================
# final
# =========================================================================================

# print all variables
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    ecbuild_info("  ${_variableName}=${${_variableName}}")
#endforeach()

# prepares a tar.gz of your sources and/or binaries
ecbuild_install_project( NAME ecFlow )

# print the summary of the configuration
ecbuild_print_summary()
