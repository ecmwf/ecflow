#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# ========================================================================
# EXE ecflow_client, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable(
  TARGET
    ecflow_client
  SOURCES
    src/ecflow/client/ClientMain.cpp
  LIBS
    ecflow_all
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
)
target_clangformat(ecflow_client)

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_client
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

add_subdirectory(test/harness)

# ================================================================================                         
# TEST
set(test_srcs
  # Sources
  test/TestClient_main.cpp # test entry point
  test/TestClientConfigurations.cpp
  test/TestClientEnvironment.cpp
  test/TestClientOptions.cpp
  test/TestClientRetry.cpp
  test/TestClientInterface.cpp
  test/TestHostsFile.cpp
  test/TestJobGenOnly.cpp
  test/TestLifeCycle.cpp
  test/TestInitAddVariables.cpp
  test/TestRtt.cpp
  test/TestUrlCmd.cpp
)

if (ENABLE_SERVER)
  list(APPEND test_srcs
    # Sources
    test/TestClientTimeout.cpp
    test/TestClientHandleCmd.cpp
    test/TestCheckPtDefsCmd.cpp
    test/TestCustomUser.cpp
    test/TestGroupCmd.cpp
    test/TestLoadDefsCmd.cpp
    test/TestLogAndCheckptErrors.cpp
    test/TestPasswdFile.cpp
    test/TestPlugCmd.cpp
    test/TestServer.cpp
    test/TestServerAndLifeCycle.cpp
    test/TestServerLoad.cpp
    test/TestSignalSIGTERM.cpp
    test/TestWhiteListFile.cpp
  )
endif()

ecbuild_add_test(
  TARGET
    s_client
  LABELS
    integration nightly
  SOURCES
    ${test_srcs}
  INCLUDES
    ../base/test
    ../node/test
  LIBS
    ecflow_all
    test_scaffold
    test_harness.base
    test_harness.client
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::timer
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  DEPENDS
    ecflow_server # the server is launched to support tests
  TEST_DEPENDS
    u_base
  CONDITION
    ENABLE_SERVER # AND ENABLE_TESTS
)
target_clangformat(s_client
  CONDITION ENABLE_SERVER AND ENABLE_TESTS
)

if (ENABLE_ALL_TESTS AND ENABLE_SERVER)
  #
  # Simple stand alone test
  #
  ecbuild_add_test(
    TARGET
      p_large_defs
    LABELS
      performance
      manual # requires manual definition of env var ECF_TEST_DEFS_DIR
    SOURCES
      # Sources
      test/TestSinglePerf.cpp
      test/TestSinglePerf_main.cpp # test entry point
    INCLUDES
      ../base/test
    LIBS
      ecflow_all
      test_scaffold
      test_harness.base
      test_harness.client
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    DEPENDS
      ecflow_server # the server is launched to support tests
    TEST_DEPENDS
      u_base
  )
  target_clangformat(p_large_defs
    CONDITION ENABLE_TESTS
  )

  #
  # test migration
  #
  ecbuild_add_test(
    TARGET
      m_migration
    LABELS
      migration
      manual # requires command line argument: m_test_migration <>
    SOURCES
      # Sources
      test/TestMigration.cpp
      test/TestMigration_main.cpp # test entry point
    INCLUDES
      ../base/test
    LIBS
      ecflow_all
      test_scaffold
      test_harness.base
      test_harness.client
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    DEPENDS
      ecflow_server # the server is launched to support tests
    TEST_DEPENDS
      u_base
  )
  target_clangformat(m_migration
    CONDITION ENABLE_TESTS
  )

endif()

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_client DESTINATION bin)
