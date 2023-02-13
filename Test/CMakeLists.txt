# =======================================================
# to list all sources to build use:
#    cd $WK/Test
#    find src -name \*.cpp -print | sort
# =======================================================
file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" "src/*.hpp" )
 
# This library is used for the syste, level testing, hence can include test directories
ecbuild_add_library( TARGET   libharness
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                    )

target_link_libraries(libharness libclient base node nodeattr core )
target_include_directories(libharness PUBLIC src
                                              ../ACore/src 
                                              ../ANattr/src
                                              ../ANode/src
                                              ../ANode/test
                                              ../Base/src
                                              ../Base/src/cts
                                              ../Base/src/stc
                                              ../Base/test
                                              ../Client/src
                                              ../Client/test 
)

target_clangformat(libharness)

# =================================================================
# test

if (ENABLE_ALL_TESTS)

	list( APPEND test_srcs
	./TestAbortCmd.cpp
	./TestAlias.cpp
	./TestClkSync.cpp
	./TestComplete.cpp
	./TestCron.cpp
    ./TestCtsWaitCmd.cpp
    ./TestDayDate.cpp
	./TestEvents.cpp
	./TestFileCmd.cpp
    ./TestHandle.cpp
    ./TestInitAddVariable.cpp
	./TestKillCmd.cpp
	./TestLate.cpp
	./TestLimit.cpp
    ./TestOrderCmd.cpp
    ./TestQueueCmd.cpp
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
	
	# if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty
	# libboost_unit_test_framework  undefined reference to `clock_gettime', ${LIBRT} needed for boost 1.71
	ecbuild_add_test( TARGET       s_test
	                  SOURCES      ${test_srcs}
	                  INCLUDES     ${Boost_INCLUDE_DIRS}
	                  LIBS         libharness ${OPENSSL_LIBRARIES}
	                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                     DEFINITIONS  ${BOOST_TEST_DYN_LINK}
	                  TEST_DEPENDS s_client
	                )
	target_clangformat(s_test)
   
	ecbuild_add_test( TARGET       s_test_zombies
	                  SOURCES      ./TestZombies.cpp
	                  INCLUDES     ${Boost_INCLUDE_DIRS}
	                  LIBS         libharness ${OPENSSL_LIBRARIES}
	                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                     DEFINITIONS  ${BOOST_TEST_DYN_LINK}
	                  TEST_DEPENDS s_test
	                )
	target_clangformat(s_test_zombies)
	
   
	# This test runs forever, hence not suitable for unit test.                          
	#ecbuild_add_test( TARGET       singletest
	#                  SOURCES      ./TestSingle.cpp
	#                  INCLUDES     ${Boost_INCLUDE_DIRS}
	#                  LIBS         libharness 
	#                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
   #                  DEFINITIONS  ${BOOST_TEST_DYN_LINK}
	#                  TEST_DEPENDS s_client
	#               )
endif()
