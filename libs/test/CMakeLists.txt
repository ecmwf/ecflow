#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

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

  ecbuild_add_test(
    TARGET
      s_test
    LABELS
      integration
      nightly
    SOURCES
      ${test_srcs}
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_client
  )
  target_clangformat(s_test CONDITION ENABLE_TESTS)

  ecbuild_add_test(
    TARGET
      s_zombies
    LABELS
      integration
      nightly
    SOURCES
      TestZombies.cpp
      TestZombies_main.cpp # test entry point
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_test
  )
  target_clangformat(s_zombies CONDITION ENABLE_TESTS)

  # This test runs forever, hence not suitable as unit test.
  ecbuild_add_test(
    TARGET
      s_single
    LABELS
      integration
      manual
    SOURCES
      TestSingle.cpp
      TestSingle_main.cpp # test entry point
    LIBS
      test_scaffold
      test_harness.test
      $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    TEST_DEPENDS
      s_client
  )
  target_clangformat(s_single CONDITION ENABLE_TESTS)

endif()
