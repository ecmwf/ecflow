#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(test_srcs
  # Headers
  test/MyDefsFixture.hpp
  # Sources
  test/TestAdd.cpp
  test/TestAlias.cpp
  test/TestAssignmentOperator.cpp
  test/TestAvisoAttr.cpp
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
  test/TestMissNextTimeSlot.cpp
  test/TestMovePeer.cpp
  test/TestNodeBeginRequeue.cpp
  test/TestNodeState.cpp
  test/TestNode_main.cpp # test entry point
  test/TestOrder.cpp
  test/TestPersistence.cpp
  test/TestPreProcessing.cpp
  test/TestRepeatWithTimeDependencies.cpp
  test/TestReplace.cpp
  test/TestSetState.cpp
  test/TestSpecificIssues.cpp
  test/TestSystem.cpp
  test/TestTaskScriptGenerator.cpp
  test/TestTimeDependencies.cpp
  test/TestVariableGeneration.cpp
  test/TestVariableInheritance.cpp
  test/TestVariableSubstitution.cpp
  test/TestVariableSubstitutionDefs.cpp
  test/TestZombies.cpp
)

ecbuild_add_test(
  TARGET
    u_node
  LABELS
    unit
    nightly
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  TEST_DEPENDS
    u_attributes
)
target_clangformat(u_node
  CONDITION ENABLE_TESTS)


set(test_srcs
  # Headers
  test/parser/PersistHelper.hpp
  test/parser/TemporaryFile.hpp
  # Sources
  test/parser/PersistHelper.cpp
  test/parser/TemporaryFile.cpp
  test/parser/TestAutoAddExterns.cpp
  test/parser/TestAvisoAttr.cpp
  test/parser/TestDefsStructurePersistAndReload.cpp
  test/parser/TestMementoPersistAndReload.cpp
  test/parser/TestMigration.cpp
  test/parser/TestMirrorAttr.cpp
  test/parser/TestParser.cpp
  test/parser/TestParser_main.cpp # test entry point
  test/parser/TestVariableParsing.cpp
)
ecbuild_add_test(
  TARGET
    u_parser
  LABELS
    unit
    nightly
  SOURCES
    ${test_srcs}
  INCLUDES
    test
    test/parser
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  TEST_DEPENDS
    u_node
)
target_clangformat(u_parser
  CONDITION ENABLE_TESTS
)

if (ENABLE_ALL_TESTS)
  set(test_srcs
    test/TestSingleExprParse.cpp
    test/TestSingleExprParse_main.cpp # test entry point
  )

  ecbuild_add_test(
    TARGET
      u_node_single
    LABELS
      unit
      nightly
    SOURCES
      ${test_srcs}
    LIBS
      ecflow_all
      test_scaffold
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    TEST_DEPENDS
      u_anattr
  )
  target_clangformat(u_node_single
    CONDITION ENABLE_TESTS
  )


  set(test_srcs
    # Headers
    test/parser/PersistHelper.hpp
    test/parser/TemporaryFile.hpp
    # Sources
    test/parser/PersistHelper.cpp
    test/parser/TemporaryFile.cpp
    test/parser/TestParserPerformance_main.cpp # test entry point
    test/parser/TestSingleDefsFile.cpp
  )

  ecbuild_add_test(
    TARGET
      p_parser
    LABELS
      performance
      nightly
    SOURCES
      ${test_srcs}
    LIBS
      ecflow_all
      test_scaffold
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  )
  target_clangformat(p_parser
    CONDITION ENABLE_TESTS
  )


  set(test_srcs
    # Headers
    test/parser/PersistHelper.hpp
    test/parser/TemporaryFile.hpp
    # Sources
    test/parser/ParseTimer.cpp
    test/parser/PersistHelper.cpp
    test/parser/TemporaryFile.cpp
  )

  # The following is not technically a test (as it makes no checks),
  # but a tool to measure the time it takes to parse 'mega.def' file
  ecbuild_add_test(
    TARGET
      p_parser_timer
    LABELS
      performance
      nightly
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/test/parser/data/single_defs/mega.def
    SOURCES
      ${test_srcs}
    LIBS
      ecflow_all
      test_scaffold
      Boost::boost # Boost header-only libraries must be available
  )
  target_clangformat(p_parser_timer
    CONDITION ENABLE_TESTS
  )

  # The following is not technically a test (as it makes no checks),
  # but a tool to parse 'mega.def' file
  set(test_srcs
    test/parser/ParseOnly.cpp
  )
  ecbuild_add_test(
    TARGET
      p_parser_only
    LABELS
      performance
      nightly
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/test/parser/data/single_defs/mega.def
    SOURCES
      ${test_srcs}
    LIBS
      ecflow_all
      test_scaffold
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  )
  target_clangformat(p_parser_only
    CONDITION ENABLE_TESTS
  )

endif()
