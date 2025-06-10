#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

add_subdirectory(test/harness)

set(test_srcs
  # Sources
  test/TestAlterCmd.cpp
  test/TestArchiveAndRestoreCmd.cpp
  test/TestBase_main.cpp # test entry point
  test/TestClientHandleCmd.cpp
  test/TestCmd.cpp
  test/TestDeleteNodeCmd.cpp
  test/TestForceCmd.cpp
  test/TestFreeDepCmd.cpp
  test/TestInLimitAndLimit.cpp
  test/TestLogCmd.cpp
  test/TestMeterCmd.cpp
  test/TestProgramOptions.cpp
  test/TestQueryCmd.cpp
  test/TestQueueCmd.cpp
  test/TestRequest.cpp
  test/TestRequeueNodeCmd.cpp
  test/TestResolveDependencies.cpp
  test/TestSpecificIssues.cpp
  test/TestSSyncCmd.cpp
  test/TestSSyncCmdOrder.cpp
  test/TestSSyncCmd_CH1.cpp
  test/TestStatsCmd.cpp
)

ecbuild_add_test(
  TARGET
    u_base
  LABELS
    unit nightly
  SOURCES
    ${test_srcs}
  INCLUDES
    ../node/test
  LIBS
    ecflow_all
    test_scaffold
    test_harness.base
    Threads::Threads
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  TEST_DEPENDS
    u_parser
)
target_clangformat(u_base
  CONDITION ENABLE_TESTS
)

# The following is not technically a test (as it makes no checks),
# but a tool to measure the time it takes to generate a job file
if (ENABLE_ALL_TESTS)
  set(test_srcs
    # Sources
    test/TestJobGenPerf.cpp
  )

  ecbuild_add_test(
    TARGET
      p_job_gen
    LABELS
      performance
    SOURCES
      ${test_srcs}
    LIBS
      ecflow_all
      test_scaffold
      test_harness.base
      Threads::Threads
  )
  target_clangformat(p_job_gen
    CONDITION ENABLE_TESTS
  )
endif()
