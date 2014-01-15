# =======================================================
# to list all sources to build use:
#    cd $WK/Test
#    find src -name \*.cpp -print | sort
# =======================================================
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
 
ecbuild_add_library( TARGET   libharness
                     NOINSTALL
                   TYPE     STATIC
                   SOURCES  ${srcs}
                   LIBS     core nodeattr node libparser base libclient ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                            ../Base/test
                            ../Client/src
                            ${Boost_INCLUDE_DIRS} )


# =================================================================
# test
list( APPEND test_srcs
./TestAbortCmd.cpp
./TestAlias.cpp
./TestClkSync.cpp
./TestComplete.cpp
./TestCron.cpp
./TestCtsWaitCmd.cpp
./TestEvents.cpp
./TestFileCmd.cpp
./TestHandle.cpp
./TestKillCmd.cpp
./TestLate.cpp
./TestLimit.cpp
./TestOrderCmd.cpp
./TestRepeat.cpp
./TestRequeueNode.cpp
./TestRunner.cpp
./TestServer.cpp
./TestSuspend.cpp
./TestToday.cpp
./TestTrigger.cpp
./TestWhyCmd.cpp
./Test_Time.cpp
)
ecbuild_add_test( TARGET server-test
                SOURCES  ${test_srcs}
                LIBS     core nodeattr node libparser base libclient libharness pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ../Client/src
                          ${Boost_INCLUDE_DIRS} )


ecbuild_add_test( TARGET  test-zombies
                SOURCES   ./TestZombies.cpp
                LIBS      core nodeattr node libparser base libclient libharness pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ../Client/src
                          ${Boost_INCLUDE_DIRS} )
                          
ecbuild_add_test( TARGET  singletest
                SOURCES   ./TestSingle.cpp
                LIBS      core nodeattr node libparser base libclient libharness pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ${Boost_INCLUDE_DIRS} )

