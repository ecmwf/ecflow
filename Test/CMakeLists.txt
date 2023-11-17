#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )

set(srcs
  # Headers
  src/ServerTestHarness.hpp
  src/TestFixture.hpp
  src/ZombieUtil.hpp
  # Sources
  src/ServerTestHarness.cpp
  src/TestFixture.cpp
  src/ZombieUtill.cpp
)

ecbuild_add_library(
  TARGET
    libharness
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
    ../Base/test
  PUBLIC_LIBS
    libclient
    base
    node
    nodeattr
    core
)
target_clangformat(libharness)

# =================================================================
# test

if (ENABLE_ALL_TESTS)

  set(test_srcs
    TestAbortCmd.cpp
    TestAlias.cpp
    TestClkSync.cpp
    TestComplete.cpp
    TestCron.cpp
    TestCtsWaitCmd.cpp
    TestDayDate.cpp
    TestEvents.cpp
    TestFileCmd.cpp
    TestHandle.cpp
    TestInitAddVariable.cpp
    TestKillCmd.cpp
    TestLate.cpp
    TestLimit.cpp
    TestOrderCmd.cpp
    TestQueueCmd.cpp
    TestRepeat.cpp
    TestRequeueNode.cpp
    TestRunner.cpp
    TestServer.cpp
    TestSuspend.cpp
    TestToday.cpp
    TestTrigger.cpp
    TestWhyCmd.cpp
    Test_Time.cpp
  )

  ecbuild_add_test(
    TARGET
      s_test
    LABELS
      integration nightly
    SOURCES
      ${test_srcs}
    LIBS
      libharness
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_client
  )
  target_clangformat(s_test CONDITION ENABLE_TESTS)
   
  ecbuild_add_test(
    TARGET
      s_test_zombies
    LABELS
      integration nightly
    SOURCES
      TestZombies.cpp
    LIBS
      libharness
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_test
  )
  target_clangformat(s_test_zombies CONDITION ENABLE_TESTS)

  # This test runs forever, hence not suitable as unit test.
  ecbuild_add_test(
    TARGET
      s_singletest
    LABELS
      integration nightly
    SOURCES
      TestSingle.cpp
    LIBS
      libharness
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_client
  )
  target_clangformat(s_singletest CONDITION ENABLE_TESTS)

endif()
