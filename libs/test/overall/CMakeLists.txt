# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(harness)

# =================================================================
# test

if (ENABLE_ALL_TESTS)

  set(test_srcs
    Test_main.cpp # test entry point
    TestAbortCmd.cpp
    TestAlias.cpp
    TestClkSync.cpp
    TestComplete.cpp
    TestCron.cpp
    TestCtsWaitCmd.cpp
    TestDayDate.cpp
    TestEcfNoScriptCmd.cpp
    TestEcfScriptCmd.cpp
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
    TestServer.cpp
    TestSpecificIssues.cpp
    TestSuspend.cpp
    TestTime.cpp
    TestToday.cpp
    TestTrigger.cpp
    TestWhyCmd.cpp
  )

  ecflow_add_test(
    TARGET
      s_test
    LABELS
      integration nightly slow
    SOURCES
      ${test_srcs}
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    DEPENDS
      ecflow_server
    TEST_DEPENDS
      s_client
    CONDITION
      ENABLE_TESTS
    FORMAT
  )

  ecflow_add_test(
    TARGET
      s_test_using_http_backend
    LABELS
      integration nightly slow
    SOURCES
      ${test_srcs}
    DEFINITIONS
      ECFLOW_TEST_SERVER_HTTP
    LIBS
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    ENVIRONMENT
      ECF_TEST_USING_HTTP=1
    DEPENDS
      ecflow_server
    TEST_DEPENDS
      s_client
    CONDITION
      ENABLE_TESTS
    FORMAT
  )

  ecflow_add_test(
    TARGET
      s_zombies
    LABELS
      integration nightly slow
    SOURCES
      TestZombies.cpp
      TestZombies_main.cpp # test entry point
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    DEPENDS
      ecflow_server
    TEST_DEPENDS
      s_test
    CONDITION
      ENABLE_TESTS
    FORMAT
  )

  # This test runs forever, hence not suitable as unit test.
  ecflow_add_test(
    TARGET
      s_single
    LABELS
      integration manual
    SOURCES
      TestSingle.cpp
      TestSingle_main.cpp # test entry point
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    DEPENDS
      ecflow_server
    TEST_DEPENDS
      s_client
    CONDITION
      ENABLE_TESTS
    FORMAT
  )

endif()
