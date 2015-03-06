# =======================================================
# to list all sources to build use:
#    cd $WK/Test
#    find src -name \*.cpp -print | sort
# =======================================================
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )
 
# This library is used for the syste, level testing, hence can include test directories
ecbuild_add_library( TARGET   libharness
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     libclient base libparser node nodeattr core  
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../ANode/test
                              ../AParser/src
                              ../Base/src
                              ../Base/src/cts
                              ../Base/src/stc
                              ../Base/test
                              ../Client/src
                              ../Client/test
                    )

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
ecbuild_add_test( TARGET s_test
                  BOOST
                  SOURCES  ${test_srcs}
                  LIBS     libharness 
                )
set_property(TEST s_test APPEND PROPERTY DEPENDS s_client)


ecbuild_add_test( TARGET  s_test_zombies
                  BOOST
                  SOURCES   ./TestZombies.cpp
                  LIBS      libharness 
                )
set_property(TEST s_test_zombies APPEND PROPERTY DEPENDS s_test)

# This test runs forever, hence not suitable for unit test.                          
#ecbuild_add_test( TARGET  singletest
#                  BOOST
#                  SOURCES   ./TestSingle.cpp
#                  LIBS      libharness 
#               )
