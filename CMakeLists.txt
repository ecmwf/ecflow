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

find_package( ecbuild 3.4 REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild /workspace/ecbuild) # Before project()

project( ecflow LANGUAGES CXX VERSION 5.13.4 )

# Important: the project version is used, as generated CMake variables, to filter .../ecflow/core/ecflow_version.h.in

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
option( ENABLE_SERVER              "Build the server (switch OFF to building UI only)"   ON )
option( ENABLE_PYTHON              "Enable ecFlow Python3 support"                       ON )
option( ENABLE_UI                  "Enable ecFlowUI"                                     ON )
option( ENABLE_STATIC_BOOST_LIBS   "Use static Boost libs linkage"                       ON )
option( ENABLE_ALL_TESTS           "Enable performance/migration/regression tests"       OFF )
option( ENABLE_UI_BACKTRACE        "Enable printing ecFlowUI debug backtrace"            OFF )
option( ENABLE_UI_USAGE_LOG        "Enable ecFlowUI usage logging"                       OFF )
option( ENABLE_SSL                 "Enable SSL encrypted communication"                  ON )
option( ENABLE_PYTHON_PTR_REGISTER "Enable compilers/Boost shared ptr auto registration" OFF )
option( ENABLE_HTTP                "Enable Rest API/HTTP server"                         ON )
option( ENABLE_HTTP_COMPRESSION    "Enable compression support by HTTP server"           ON )
option( ENABLE_UDP                 "Enable UDP server"                                   ON )
option( ENABLE_DOCS                "Enable Documentation"                                OFF )

include(Policies NO_POLICY_SCOPE)


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
ecbuild_info( "ENABLE_UI                  : ${ENABLE_UI}" )
ecbuild_info( "ENABLE_TESTS               : ${ENABLE_TESTS} *if* disabled no need for boost test libs" )
ecbuild_info( "ENABLE_ALL_TESTS           : ${ENABLE_ALL_TESTS}" )
ecbuild_info( "ENABLE_STATIC_BOOST_LIBS   : ${ENABLE_STATIC_BOOST_LIBS}" )
ecbuild_info( "ENABLE_SSL                 : ${ENABLE_SSL} *if* openssl libraries available" )
ecbuild_info( "ENABLE_HTTP                : ${ENABLE_HTTP}" )
ecbuild_info( "ENABLE_HTTP_COMPRESSION    : ${ENABLE_HTTP_COMPRESSION}" )
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
# enable clang-format
# =========================================================================================
find_package(ClangFormat)


# =========================================================================================
# build source code
# =========================================================================================
add_subdirectory( libs )

if (ENABLE_SERVER)
  add_subdirectory( tools )
endif()

if (ENABLE_UI)
  add_subdirectory( Viewer )
  add_subdirectory( share )
endif()


# =========================================================================================
# tar ball, by default ecbuild will tar everything apart from hard wired directory called 'build' 
# hence we can tell it, what directories not to pack
# =========================================================================================
ecbuild_dont_pack(
  DIRS
    .scratch
    .git
    .settings
    bamboo
    libs/core/doc
    libs/attributes/doc
    libs/node/doc
    libs/client/doc
    libs/simulator/doc
    libs/pyext/doc
    libs/server/doc
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
