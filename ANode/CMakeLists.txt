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
 test/TestDefs.cpp
 test/TestPersistence.cpp
 test/TestVariableSubstitution.cpp
 test/TestFlag.cpp
 test/TestEnviromentSubstitution.cpp
 test/TestLimit.cpp
 test/TestSetState.cpp
 test/TestVariableInheritance.cpp
 test/TestDefStatus.cpp
 test/TestExprRepeatDateArithmetic.cpp
 test/TestJobCreator.cpp
 test/TestSmsLocator.cpp
 test/TestFindAbsNodePath.cpp
 test/TestTaskScriptGenerator.cpp
 test/TestHistoryParser.cpp
 test/TestVariableGeneration.cpp
 test/TestEcfFile.cpp
 test/TestReplace.cpp
 test/TestExprParser.cpp
 test/TestPreProcessing.cpp
 test/TestOrder.cpp
 test/TestInLimit.cpp
 test/TestAlias.cpp
 test/TestZombies.cpp
 test/TestMigration.cpp
)

ecbuild_add_test(TARGET  u_anode
                 BOOST
                 SOURCES  ${test_srcs}
                 LIBS     node  
                 )


list( APPEND stest_srcs   test/TestSingleExprParse.cpp )                    
ecbuild_add_test( TARGET  u_anode_stest
                  BOOST
                  SOURCES   ${stest_srcs}
                  LIBS      node 
                  INCLUDES  ${Boost_INCLUDE_DIRS} 
                 )
