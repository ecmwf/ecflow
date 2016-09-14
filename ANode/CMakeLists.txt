# Note:
# If new src or test cpp files are added make sure you touch this file
#

file( GLOB srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cpp" )

ecbuild_add_library( TARGET   node
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     nodeattr core
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                    )

# Use following to populate list: 
#  cd $WK/ANode                      
#  find test -name \*.cpp | sort      

# no way to exclude file ?
#file( GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test/*.cpp" )
 
list( APPEND test_srcs
test/Test_ECFLOW-195.cpp
test/Test_ECFLOW-247.cpp
test/Test_ECFLOW-417.cpp
test/TestAdd.cpp
test/TestAlias.cpp
test/TestAssignmentOperator.cpp
test/TestChangeMgrSingleton.cpp
test/TestCopyConstructor.cpp
test/TestDefStatus.cpp
test/TestDefs.cpp
test/TestEcfFile.cpp
test/TestEnviromentSubstitution.cpp
test/TestExprParser.cpp
test/TestExprRepeatDateArithmetic.cpp
test/TestFindAbsNodePath.cpp
test/TestFlag.cpp
test/TestHistoryParser.cpp
test/TestInLimit.cpp
test/TestJobCreator.cpp
test/TestJobProfiler.cpp
test/TestLimit.cpp
test/TestMigration.cpp
test/TestMissNextTimeSlot.cpp
test/TestNodeBeginReque.cpp
test/TestNodeState.cpp
test/TestOrder.cpp
test/TestPersistence.cpp
test/TestPreProcessing.cpp
test/TestReplace.cpp
test/TestSetState.cpp
test/TestSmsLocator.cpp
test/TestTaskScriptGenerator.cpp
test/TestTimeDependencies.cpp
test/TestVariableGeneration.cpp
test/TestVariableInheritance.cpp
test/TestVariableSubstitution.cpp
test/TestVariableSubstitutionDefs.cpp
test/TestZombies.cpp
)

ecbuild_add_test(TARGET  u_anode
                 BOOST
                 SOURCES      ${test_srcs}
                 LIBS         node  
                 TEST_DEPENDS u_anattr
                 )

if (ENABLE_ALL_TESTS)
	list( APPEND stest_srcs   test/TestSingleExprParse.cpp )                    
	ecbuild_add_test( TARGET       u_anode_stest
                      BOOST
                      SOURCES      ${stest_srcs}
                      LIBS         node 
                      TEST_DEPENDS u_anattr
                    )
endif()

