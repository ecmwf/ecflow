# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# =======================================================
# EXE ecflow_server, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecflow_add_executable(
  TARGET
    ecflow_server
  SOURCES
    src/ecflow/server/ServerMain.cpp
  LIBS
    ecflow_all
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
    $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
  DEFINITIONS
    $<$<BOOL:${ZLIB_FOUND}>:ECF_HTTP_COMPRESSION>
  FORMAT
)

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#
SET_TARGET_PROPERTIES(ecflow_server
  PROPERTIES
    INSTALL_RPATH ""
)

# use, i.e. don't skip the full RPATH for the build tree
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# the RPATH to be used when installing
#SET(CMAKE_INSTALL_RPATH "")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)


# ================================================================================
# TEST
ecflow_add_test(
  TARGET
    u_server
  LABELS
    unit nightly fast
  INCLUDES
    src
  SOURCES
    test/TestCheckPtSaver.cpp
    test/TestServerConfigurations.cpp
    test/TestServerEnvironment.cpp
    test/TestServer_main.cpp # test entry point
    test/TestServer.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
  TEST_DEPENDS
    u_base
  CONDITION
    ENABLE_TESTS
  FORMAT
)

ecflow_add_test(
  TARGET
    u_server_authentication
  LABELS
    unit nightly fast
  SOURCES
    test/TestAuthentication.cpp
    test/TestAuthentication_main.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  TEST_DEPENDS
    u_base
  CONDITION
    ENABLE_TESTS
  FORMAT
)

# ================================================================================
# TEST
ecflow_add_test(
  TARGET
    s_periodic_scheduler
  LABELS
    integration nightly slow
  INCLUDES
    src
  SOURCES
    test/TestPeriodicScheduler.cpp
    test/TestPeriodicScheduler_main.cpp # test entry point
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
  TEST_DEPENDS
    u_base
  CONDITION
    ENABLE_TESTS
  FORMAT
)

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_server DESTINATION bin)
