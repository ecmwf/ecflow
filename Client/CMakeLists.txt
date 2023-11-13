#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/ClientCmdCache.hpp
  src/ClientEnvironment.hpp
  src/ClientInvoker.hpp
  src/ClientOptions.hpp
  src/Help.hpp
  src/Rtt.hpp
  src/UrlCmd.hpp
  # Sources
  src/ClientCmdCache.cpp
  src/Rtt.cpp
  src/ClientEnvironment.cpp
  src/ClientInvoker.cpp
  src/Help.cpp
  src/Rtt.cpp
  src/ClientOptions.cpp
  src/UrlCmd.cpp
)

ecbuild_add_library(
  TARGET
    libclient
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    base
    Threads::Threads
)
target_clangformat(libclient)

# ========================================================================
# EXE ecflow_client, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable(
  TARGET
    ecflow_client
  SOURCES
    src/ClientMain.cpp
  LIBS
    libclient
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


# ================================================================================                         
# TEST
set(test_srcs
  # Headers
  test/InvokeServer.hpp
  # Sources
  test/TestClient_main.cpp # test entry point
  test/TestClientEnvironment.cpp
  test/TestClientOptions.cpp
  test/TestClientInterface.cpp
  test/TestJobGenOnly.cpp
  test/TestLifeCycle.cpp
  test/TestInitAddVariables.cpp
  test/TestRtt.cpp
  test/TestUrlCmd.cpp
)

if (ENABLE_SERVER)
  list(APPEND test_srcs
      # Headers
      test/SCPort.hpp
      # Sources
      test/SCPort.cpp
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
  SOURCES
    ${test_srcs}
  INCLUDES
    ../Base/test
    ../ANode/test
  LIBS
    libclient
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::timer
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  TEST_DEPENDS
    u_base
)
target_clangformat(s_client
  CONDITION ENABLE_TESTS
)

if (ENABLE_ALL_TESTS AND ENABLE_SERVER)
  #
  # Simple stand alone test
  #
  ecbuild_add_test(
    TARGET
      perf_test_large_defs
    SOURCES
      # Headers
      test/SCPort.hpp
      # Sources
      test/SCPort.cpp
      test/TestSinglePerf.cpp
      test/TestSinglePerf_main.cpp # test entry point
    INCLUDES
      ../Base/test
    LIBS
      libclient
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      u_base
  )
  target_clangformat(perf_test_large_defs
    CONDITION ENABLE_TESTS
  )

  #
  # test migration
  #
  ecbuild_add_test(
    TARGET
      m_test_migration
    SOURCES
      # Headers
      test/SCPort.hpp
      # Sources
      test/SCPort.cpp
      test/TestMigration.cpp
      test/TestMigration_main.cpp # test entry point
    INCLUDES
      ../Base/test
    LIBS
      libclient
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      u_base
  )
  target_clangformat(m_test_migration
    CONDITION ENABLE_TESTS
  )

endif()

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_client DESTINATION bin)
