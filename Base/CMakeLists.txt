 # =======================================================
 # to list all sources to build use:
 #    cd $WK/Base
 #    find src -name \*.cpp >> CMakeLists.txt
 # =======================================================
 list( APPEND srcs
 src/ZombieCtrl.cpp
 src/Stats.cpp
 src/Client.cpp
 src/ServerReply.cpp
 src/Connection.cpp
 src/stc/BlockClientZombieCmd.cpp
 src/stc/DefsCache.cpp
 src/stc/DefsCmd.cpp
 src/stc/PreAllocatedReply.cpp
 src/stc/SStringVecCmd.cpp
 src/stc/StcCmd.cpp
 src/stc/SSuitesCmd.cpp
 src/stc/SClientHandleCmd.cpp
 src/stc/SStringCmd.cpp
 src/stc/ServerToClientCmd.cpp
 src/stc/SClientHandleSuitesCmd.cpp
 src/stc/SServerLoadCmd.cpp
 src/stc/SNodeCmd.cpp
 src/stc/SStatsCmd.cpp
 src/stc/SSyncCmd.cpp
 src/stc/SNewsCmd.cpp
 src/stc/ErrorCmd.cpp
 src/stc/GroupSTCCmd.cpp
 src/stc/ZombieGetCmd.cpp
 src/ClientToServerRequest.cpp
 src/Gnuplot.cpp
 src/WhyCmd.cpp
 src/ServerToClientResponse.cpp
 src/cts/CSyncCmd.cpp
 src/cts/ZombieCmd.cpp
 src/cts/OrderNodeCmd.cpp
 src/cts/CheckPtCmd.cpp
 src/cts/CtsNodeCmd.cpp
 src/cts/GroupCTSCmd.cpp
 src/cts/UserCmd.cpp
 src/cts/ClientHandleCmd.cpp
 src/cts/PlugCmd.cpp
 src/cts/QueryCmd.cpp
 src/cts/ForceCmd.cpp
 src/cts/LoadDefsCmd.cpp
 src/cts/CFileCmd.cpp
 src/cts/ShowCmd.cpp
 src/cts/CtsCmd.cpp
 src/cts/TaskApi.cpp
 src/cts/EditScriptCmd.cpp
 src/cts/FreeDepCmd.cpp
 src/cts/PathsCmd.cpp
 src/cts/DeleteCmd.cpp
 src/cts/EditHistoryMgr.cpp
 src/cts/LogMessageCmd.cpp
 src/cts/CtsApi.cpp
 src/cts/ReplaceNodeCmd.cpp
 src/cts/RequeueNodeCmd.cpp
 src/cts/BeginCmd.cpp
 src/cts/LogCmd.cpp
 src/cts/AlterCmd.cpp
 src/cts/TaskCmds.cpp
 src/cts/CtsCmdRegistry.cpp
 src/cts/RunNodeCmd.cpp
 src/cts/ClientToServerCmd.cpp
 src/cts/ServerVersionCmd.cpp
)

if(OPENSSL_FOUND)
  list( APPEND srcs 
     src/Openssl.cpp 
     src/ssl_connection.cpp
     src/SslClient.cpp
  )
endif()

ecbuild_add_library( TARGET   base
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     )

# This ensures that for debug config, we only link with debug boost libs, for other configs, we link with optimised boost libs
target_link_libraries(base node nodeattr core ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})
target_include_directories(base PUBLIC src
                              ../ACore/src
                              ../ANattr/src
                              ../ANode/src
                              ../Base/src
                              ../Base/src/cts
                              ../Base/src/stc )


list( APPEND test_srcs
   test/TestAlterCmd.cpp
   test/TestArchiveAndRestoreCmd.cpp
   test/TestClientHandleCmd.cpp
   test/TestCmd.cpp
   test/TestDeleteNodeCmd.cpp
   test/TestECFLOW-189.cpp
   test/TestForceCmd.cpp
   test/TestFreeDepCmd.cpp
   test/TestInLimitAndLimit.cpp
   test/TestLogCmd.cpp
   test/TestMeterCmd.cpp
   test/TestQueryCmd.cpp
   test/TestQueueCmd.cpp
   test/TestProgramOptions.cpp
   test/TestRequest.cpp
   test/TestRequeueNodeCmd.cpp
   test/TestResolveDependencies.cpp
   test/TestSSyncCmd_CH1.cpp
   test/TestSSyncCmd.cpp
   test/TestSSyncCmdOrder.cpp
)

# if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty
# libboost_unit_test_framework  undefined reference to `clock_gettime', ${LIBRT} needed for boost 1.71
ecbuild_add_test( TARGET       u_base
                  SOURCES      ${test_srcs}
                  LIBS         base pthread ${OPENSSL_LIBRARIES}
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                  INCLUDES     ../ANode/test ${Boost_INCLUDE_DIRS}
                  DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                  TEST_DEPENDS u_aparser
                  )

                  
if (ENABLE_ALL_TESTS)
   ecbuild_add_test( TARGET       perf_job_gen 
                     SOURCES      test/TestJobGenPerf.cpp
                     LIBS         base nodeattr node core pthread                                    
                     INCLUDES     ../ANode/test ${Boost_INCLUDE_DIRS}
                     DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                  )
endif()
                  