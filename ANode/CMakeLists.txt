# Note:
# If new src or test cpp files are added make sure you touch this file
#

file(GLOB srcs
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "src/*.cpp" "src/*.hpp" "parser/src/*.cpp" "parser/src/*.hpp")

ecbuild_add_library( TARGET   node
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                    )
target_link_libraries(node PRIVATE nodeattr core)
target_include_directories(node PUBLIC ../ACore/src 
                                       ../ANattr/src 
                                       src 
                                       parser/src)

target_clangformat(node)

add_subdirectory( parser )

# Use following to populate list: 
#  cd $WK/ANode                      
#  find test -name \*.cpp | sort      

# no way to exclude file ?
#file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )
 
list( APPEND test_srcs
# HEADERS
test/MyDefsFixture.hpp
# SOURCES
test/Test_ECFLOW-195.cpp
test/Test_ECFLOW-247.cpp
test/Test_ECFLOW-417.cpp
test/TestAdd.cpp
test/TestAlias.cpp
test/TestAssignmentOperator.cpp
test/TestChangeMgrSingleton.cpp
test/TestClientSuiteMgr.cpp
test/TestCopyConstructor.cpp
test/TestDefStatus.cpp
test/TestDefs.cpp
test/TestEcfFile.cpp
test/TestEcfFileLocator.cpp
test/TestEnviromentSubstitution.cpp
test/TestExprParser.cpp
test/TestExprRepeatDateArithmetic.cpp
test/TestExprRepeatDateListArithmetic.cpp
test/TestFindAbsNodePath.cpp
test/TestFlag.cpp
test/TestHistoryParser.cpp
test/TestInLimit.cpp
test/TestJobCreator.cpp
test/TestJobProfiler.cpp
test/TestLimit.cpp
test/TestMigration.cpp
test/TestMovePeer.cpp
test/TestMissNextTimeSlot.cpp
test/TestNodeBeginReque.cpp
test/TestNodeState.cpp
test/TestOrder.cpp
test/TestPersistence.cpp
test/TestPreProcessing.cpp
test/TestRepeatWithTimeDependencies.cpp
test/TestReplace.cpp
test/TestSetState.cpp
test/TestSystem.cpp
test/TestTaskScriptGenerator.cpp
test/TestTimeDependencies.cpp
test/TestVariableGeneration.cpp
test/TestVariableInheritance.cpp
test/TestVariableSubstitution.cpp
test/TestVariableSubstitutionDefs.cpp
test/TestZombies.cpp
)

# libboost_unit_test_framework  undefined reference to `clock_gettime', ${LIBRT} needed for boost 1.71
ecbuild_add_test(TARGET  u_anode
                 SOURCES      ${test_srcs}
                 INCLUDES     ${Boost_INCLUDE_DIRS}
                 LIBS         node ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                 DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                 TEST_DEPENDS u_anattr
                 )

target_clangformat(u_anode)

if (ENABLE_ALL_TESTS)
	list( APPEND stest_srcs   test/TestSingleExprParse.cpp )                    
	ecbuild_add_test( TARGET       u_anode_stest
                      SOURCES      ${stest_srcs}
                      INCLUDES     ${Boost_INCLUDE_DIRS}
                      LIBS         node ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY} ${LIBRT}
                      DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                      TEST_DEPENDS u_anattr
                    )
  target_clangformat(u_anode_stest)
endif()
