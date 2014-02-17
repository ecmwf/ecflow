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
 src/cts/ForceCmd.cpp
 src/cts/LoadDefsCmd.cpp
 src/cts/CFileCmd.cpp
 src/cts/ShowCmd.cpp
 src/cts/CtsCmd.cpp
 src/cts/TaskApi.cpp
 src/cts/EditScriptCmd.cpp
 src/cts/FreeDepCmd.cpp
 src/cts/PathsCmd.cpp
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

ecbuild_add_library( TARGET   base
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     libparser node nodeattr core 
                              ${Boost_PROGRAM_OPTIONS_LIBRARY}    
                     INCLUDES ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../AParser/src
                              ../Base/src
                              ../Base/src/cts
                              ../Base/src/stc
                              ${Boost_INCLUDE_DIRS} )

 
list( APPEND test_srcs
   test/TestAlterCmd.cpp
   test/TestClientHandleCmd.cpp
   test/TestCmd.cpp
   test/TestDeleteNodeCmd.cpp
   test/TestForceCmd.cpp
   test/TestFreeDepCmd.cpp
   test/TestLimit.cpp
   test/TestMeterCmd.cpp
   test/TestRequest.cpp
   test/TestRequeueNodeCmd.cpp
   test/TestResolveDependencies.cpp
   test/TestSSyncCmd_CH1.cpp
   test/TestSSyncCmd.cpp
   test/TestSSyncCmdOrder.cpp
)
ecbuild_add_test( TARGET  u_base
                  BOOST
                SOURCES   ${test_srcs}
                LIBS       base       
                           pthread 
                 INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ../Base/src/cts
                          ../Base/src/stc
                          ${Boost_INCLUDE_DIRS} 
                  )
