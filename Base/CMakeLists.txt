#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/ecflow/base/AbstractClientEnv.hpp
  src/ecflow/base/AbstractServer.hpp
  src/ecflow/base/Client.hpp
  src/ecflow/base/ClientOptionsParser.hpp
  src/ecflow/base/ClientToServerRequest.hpp
  src/ecflow/base/Cmd.hpp
  src/ecflow/base/Connection.hpp
  src/ecflow/base/Gnuplot.hpp
  src/ecflow/base/ServerReply.hpp
  src/ecflow/base/ServerToClientResponse.hpp
  src/ecflow/base/Stats.hpp
  src/ecflow/base/WhyCmd.hpp
  src/ecflow/base/ZombieCtrl.hpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/Openssl.hpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/ssl_connection.hpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/SslClient.hpp>
  src/ecflow/base/cts/ClientToServerCmd.hpp
  src/ecflow/base/cts/CtsCmdRegistry.hpp
  src/ecflow/base/cts/EditHistoryMgr.hpp
  src/ecflow/base/cts/task/AbortCmd.hpp
  src/ecflow/base/cts/task/CompleteCmd.hpp
  src/ecflow/base/cts/task/CtsWaitCmd.hpp
  src/ecflow/base/cts/task/EventCmd.hpp
  src/ecflow/base/cts/task/InitCmd.hpp
  src/ecflow/base/cts/task/LabelCmd.hpp
  src/ecflow/base/cts/task/MeterCmd.hpp
  src/ecflow/base/cts/task/QueueCmd.hpp
  src/ecflow/base/cts/task/TaskApi.hpp
  src/ecflow/base/cts/task/TaskCmd.hpp
  src/ecflow/base/cts/user/CtsApi.hpp
  src/ecflow/base/cts/user/UserCmd.hpp
  src/ecflow/base/stc/BlockClientZombieCmd.hpp
  src/ecflow/base/stc/DefsCache.hpp
  src/ecflow/base/stc/DefsCmd.hpp
  src/ecflow/base/stc/ErrorCmd.hpp
  src/ecflow/base/stc/GroupSTCCmd.hpp
  src/ecflow/base/stc/PreAllocatedReply.hpp
  src/ecflow/base/stc/SClientHandleCmd.hpp
  src/ecflow/base/stc/SClientHandleSuitesCmd.hpp
  src/ecflow/base/stc/SNewsCmd.hpp
  src/ecflow/base/stc/SNodeCmd.hpp
  src/ecflow/base/stc/SServerLoadCmd.hpp
  src/ecflow/base/stc/SStatsCmd.hpp
  src/ecflow/base/stc/SStringCmd.hpp
  src/ecflow/base/stc/SStringVecCmd.hpp
  src/ecflow/base/stc/SSuitesCmd.hpp
  src/ecflow/base/stc/SSyncCmd.hpp
  src/ecflow/base/stc/ServerToClientCmd.hpp
  src/ecflow/base/stc/StcCmd.hpp
  src/ecflow/base/stc/ZombieGetCmd.hpp
  # Sources
  src/ecflow/base/Client.cpp
  src/ecflow/base/ClientOptionsParser.cpp
  src/ecflow/base/ClientToServerRequest.cpp
  src/ecflow/base/Connection.cpp
  src/ecflow/base/Gnuplot.cpp
  src/ecflow/base/ServerReply.cpp
  src/ecflow/base/ServerToClientResponse.cpp
  src/ecflow/base/Stats.cpp
  src/ecflow/base/WhyCmd.cpp
  src/ecflow/base/ZombieCtrl.cpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/Openssl.cpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/ssl_connection.cpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/base/SslClient.cpp>
  src/ecflow/base/cts/ClientToServerCmd.cpp
  src/ecflow/base/cts/CtsCmdRegistry.cpp
  src/ecflow/base/cts/EditHistoryMgr.cpp
  src/ecflow/base/cts/task/AbortCmd.cpp
  src/ecflow/base/cts/task/CompleteCmd.cpp
  src/ecflow/base/cts/task/CtsWaitCmd.cpp
  src/ecflow/base/cts/task/EventCmd.cpp
  src/ecflow/base/cts/task/InitCmd.cpp
  src/ecflow/base/cts/task/LabelCmd.cpp
  src/ecflow/base/cts/task/MeterCmd.cpp
  src/ecflow/base/cts/task/QueueCmd.cpp
  src/ecflow/base/cts/task/TaskApi.cpp
  src/ecflow/base/cts/task/TaskCmd.cpp
  src/ecflow/base/cts/user/AlterCmd.cpp
  src/ecflow/base/cts/user/BeginCmd.cpp
  src/ecflow/base/cts/user/CFileCmd.cpp
  src/ecflow/base/cts/user/CSyncCmd.cpp
  src/ecflow/base/cts/user/CheckPtCmd.cpp
  src/ecflow/base/cts/user/ClientHandleCmd.cpp
  src/ecflow/base/cts/user/CtsApi.cpp
  src/ecflow/base/cts/user/CtsCmd.cpp
  src/ecflow/base/cts/user/CtsNodeCmd.cpp
  src/ecflow/base/cts/user/DeleteCmd.cpp
  src/ecflow/base/cts/user/EditScriptCmd.cpp
  src/ecflow/base/cts/user/ForceCmd.cpp
  src/ecflow/base/cts/user/FreeDepCmd.cpp
  src/ecflow/base/cts/user/GroupCTSCmd.cpp
  src/ecflow/base/cts/user/LoadDefsCmd.cpp
  src/ecflow/base/cts/user/LogCmd.cpp
  src/ecflow/base/cts/user/LogMessageCmd.cpp
  src/ecflow/base/cts/user/MoveCmd.cpp
  src/ecflow/base/cts/user/OrderNodeCmd.cpp
  src/ecflow/base/cts/user/PathsCmd.cpp
  src/ecflow/base/cts/user/PlugCmd.cpp
  src/ecflow/base/cts/user/QueryCmd.cpp
  src/ecflow/base/cts/user/ReplaceNodeCmd.cpp
  src/ecflow/base/cts/user/RequeueNodeCmd.cpp
  src/ecflow/base/cts/user/RunNodeCmd.cpp
  src/ecflow/base/cts/user/ServerVersionCmd.cpp
  src/ecflow/base/cts/user/ShowCmd.cpp
  src/ecflow/base/cts/user/UserCmd.cpp
  src/ecflow/base/cts/user/ZombieCmd.cpp
  src/ecflow/base/stc/BlockClientZombieCmd.cpp
  src/ecflow/base/stc/DefsCache.cpp
  src/ecflow/base/stc/DefsCmd.cpp
  src/ecflow/base/stc/ErrorCmd.cpp
  src/ecflow/base/stc/GroupSTCCmd.cpp
  src/ecflow/base/stc/PreAllocatedReply.cpp
  src/ecflow/base/stc/SClientHandleCmd.cpp
  src/ecflow/base/stc/SClientHandleSuitesCmd.cpp
  src/ecflow/base/stc/SNewsCmd.cpp
  src/ecflow/base/stc/SNodeCmd.cpp
  src/ecflow/base/stc/SServerLoadCmd.cpp
  src/ecflow/base/stc/SStatsCmd.cpp
  src/ecflow/base/stc/SStringCmd.cpp
  src/ecflow/base/stc/SStringVecCmd.cpp
  src/ecflow/base/stc/SSuitesCmd.cpp
  src/ecflow/base/stc/SSyncCmd.cpp
  src/ecflow/base/stc/ServerToClientCmd.cpp
  src/ecflow/base/stc/StcCmd.cpp
  src/ecflow/base/stc/ZombieGetCmd.cpp
)

ecbuild_add_library(
  TARGET
    base
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    node
    Boost::boost # Boost header-only libraries must be available (namely asio)
)
target_clangformat(base)


set(test_srcs
  # Headers
  test/MockServer.hpp
  test/TestHelper.hpp
  # Sources
  test/TestAlterCmd.cpp
  test/TestArchiveAndRestoreCmd.cpp
  test/TestBase_main.cpp # test entry point
  test/TestClientHandleCmd.cpp
  test/TestCmd.cpp
  test/TestDeleteNodeCmd.cpp
  test/TestECFLOW-189.cpp
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
    ../ANode/test
  LIBS
    base
    Threads::Threads
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  TEST_DEPENDS
    u_aparser
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
      perf_job_gen
    LABELS
      performance
    SOURCES
      ${test_srcs}
    LIBS
      base
      Threads::Threads
  )
  target_clangformat(perf_job_gen
    CONDITION ENABLE_TESTS
  )
endif()
