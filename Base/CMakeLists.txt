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
  src/AbstractClientEnv.hpp
  src/AbstractServer.hpp
  src/Client.hpp
  src/ClientOptionsParser.hpp
  src/ClientToServerRequest.hpp
  src/Cmd.hpp
  src/Connection.hpp
  src/Gnuplot.hpp
  src/ServerReply.hpp
  src/ServerToClientResponse.hpp
  src/Stats.hpp
  src/WhyCmd.hpp
  src/ZombieCtrl.hpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/Openssl.hpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ssl_connection.hpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslClient.hpp>
  src/cts/ClientToServerCmd.hpp
  src/cts/CtsApi.hpp
  src/cts/CtsCmdRegistry.hpp
  src/cts/EditHistoryMgr.hpp
  src/cts/TaskApi.hpp
  src/stc/BlockClientZombieCmd.hpp
  src/stc/DefsCache.hpp
  src/stc/DefsCmd.hpp
  src/stc/ErrorCmd.hpp
  src/stc/GroupSTCCmd.hpp
  src/stc/PreAllocatedReply.hpp
  src/stc/SClientHandleCmd.hpp
  src/stc/SClientHandleSuitesCmd.hpp
  src/stc/SNewsCmd.hpp
  src/stc/SNodeCmd.hpp
  src/stc/SServerLoadCmd.hpp
  src/stc/SStatsCmd.hpp
  src/stc/SStringCmd.hpp
  src/stc/SStringVecCmd.hpp
  src/stc/SSuitesCmd.hpp
  src/stc/SSyncCmd.hpp
  src/stc/ServerToClientCmd.hpp
  src/stc/StcCmd.hpp
  src/stc/ZombieGetCmd.hpp
  # Sources
  src/Client.cpp
  src/ClientOptionsParser.cpp
  src/ClientToServerRequest.cpp
  src/Connection.cpp
  src/Gnuplot.cpp
  src/ServerReply.cpp
  src/ServerToClientResponse.cpp
  src/Stats.cpp
  src/WhyCmd.cpp
  src/ZombieCtrl.cpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/Openssl.cpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/ssl_connection.cpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslClient.cpp>
  src/cts/AlterCmd.cpp
  src/cts/BeginCmd.cpp
  src/cts/CFileCmd.cpp
  src/cts/CSyncCmd.cpp
  src/cts/CheckPtCmd.cpp
  src/cts/ClientHandleCmd.cpp
  src/cts/ClientToServerCmd.cpp
  src/cts/CtsApi.cpp
  src/cts/CtsCmd.cpp
  src/cts/CtsCmdRegistry.cpp
  src/cts/CtsNodeCmd.cpp
  src/cts/DeleteCmd.cpp
  src/cts/EditHistoryMgr.cpp
  src/cts/EditScriptCmd.cpp
  src/cts/ForceCmd.cpp
  src/cts/FreeDepCmd.cpp
  src/cts/GroupCTSCmd.cpp
  src/cts/LoadDefsCmd.cpp
  src/cts/LogCmd.cpp
  src/cts/LogMessageCmd.cpp
  src/cts/OrderNodeCmd.cpp
  src/cts/PathsCmd.cpp
  src/cts/PlugCmd.cpp
  src/cts/QueryCmd.cpp
  src/cts/ReplaceNodeCmd.cpp
  src/cts/RequeueNodeCmd.cpp
  src/cts/RunNodeCmd.cpp
  src/cts/ServerVersionCmd.cpp
  src/cts/ShowCmd.cpp
  src/cts/TaskApi.cpp
  src/cts/TaskCmds.cpp
  src/cts/UserCmd.cpp
  src/cts/ZombieCmd.cpp
  src/stc/BlockClientZombieCmd.cpp
  src/stc/DefsCache.cpp
  src/stc/DefsCmd.cpp
  src/stc/ErrorCmd.cpp
  src/stc/GroupSTCCmd.cpp
  src/stc/PreAllocatedReply.cpp
  src/stc/SClientHandleCmd.cpp
  src/stc/SClientHandleSuitesCmd.cpp
  src/stc/SNewsCmd.cpp
  src/stc/SNodeCmd.cpp
  src/stc/SServerLoadCmd.cpp
  src/stc/SStatsCmd.cpp
  src/stc/SStringCmd.cpp
  src/stc/SStringVecCmd.cpp
  src/stc/SSuitesCmd.cpp
  src/stc/SSyncCmd.cpp
  src/stc/ServerToClientCmd.cpp
  src/stc/StcCmd.cpp
  src/stc/ZombieGetCmd.cpp
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
    src/cts
    src/stc
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
  SOURCES
    ${test_srcs}
  INCLUDES
    ../ANode/test
  LIBS
    base
    pthread
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
    SOURCES
      ${test_srcs}
    LIBS
      base
      pthread
  )
  target_clangformat(perf_job_gen
    CONDITION ENABLE_TESTS
  )
endif()
