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
  src/AbstractObserver.hpp
  src/Alias.hpp
  src/Aspect.hpp
  src/Attr.hpp
  src/AutoRestoreAttr.hpp
  src/ClientSuiteMgr.hpp
  src/ClientSuites.hpp
  src/CmdContext.hpp
  src/Defs.hpp
  src/DefsDelta.hpp
  src/EcfFile.hpp
  src/ExprAst.hpp
  src/ExprAstVisitor.hpp
  src/ExprDuplicate.hpp
  src/ExprParser.hpp
  src/Expression.hpp
  src/Family.hpp
  src/Flag.hpp
  src/InLimit.hpp
  src/InLimitMgr.hpp
  src/JobCreationCtrl.hpp
  src/JobProfiler.hpp
  src/Jobs.hpp
  src/JobsParam.hpp
  src/Limit.hpp
  src/LimitFwd.hpp
  src/Memento.hpp
  src/MiscAttrs.hpp
  src/Node.hpp
  src/NodeContainer.hpp
  src/NodeFwd.hpp
  src/NodeState.hpp
  src/NodeStats.hpp
  src/NodeTreeVisitor.hpp
  src/ResolveExternsVisitor.hpp
  src/ServerState.hpp
  src/Signal.hpp
  src/Submittable.hpp
  src/Suite.hpp
  src/SuiteChanged.hpp
  src/System.hpp
  src/Task.hpp
  src/TaskScriptGenerator.hpp
  src/move_peer.hpp
  parser/src/AutoArchiveParser.hpp
  parser/src/AutoCancelParser.hpp
  parser/src/AutoRestoreParser.hpp
  parser/src/CalendarParser.hpp
  parser/src/ClockParser.hpp
  parser/src/CronParser.hpp
  parser/src/DateParser.hpp
  parser/src/DayParser.hpp
  parser/src/DefsParser.hpp
  parser/src/DefsStateParser.hpp
  parser/src/DefsStatusParser.hpp
  parser/src/DefsStructureParser.hpp
  parser/src/EventParser.hpp
  parser/src/ExternParser.hpp
  parser/src/GenericParser.hpp
  parser/src/InlimitParser.hpp
  parser/src/LabelParser.hpp
  parser/src/LateParser.hpp
  parser/src/LimitParser.hpp
  parser/src/MeterParser.hpp
  parser/src/Parser.hpp
  parser/src/QueueParser.hpp
  parser/src/RepeatParser.hpp
  parser/src/TimeParser.hpp
  parser/src/TodayParser.hpp
  parser/src/TriggerParser.hpp
  parser/src/VariableParser.hpp
  parser/src/VerifyParser.hpp
  parser/src/ZombieAttrParser.hpp
  # Sources
  src/Alias.cpp
  src/Attr.cpp
  src/AutoRestoreAttr.cpp
  src/ClientSuiteMgr.cpp
  src/ClientSuites.cpp
  src/CmdContext.cpp
  src/Defs.cpp
  src/DefsDelta.cpp
  src/EcfFile.cpp
  src/ExprAst.cpp
  src/ExprAstVisitor.cpp
  src/ExprDuplicate.cpp
  src/ExprParser.cpp
  src/Expression.cpp
  src/Family.cpp
  src/Flag.cpp
  src/InLimit.cpp
  src/InLimitMgr.cpp
  src/JobCreationCtrl.cpp
  src/JobProfiler.cpp
  src/Jobs.cpp
  src/JobsParam.cpp
  src/Limit.cpp
  src/Memento.cpp
  src/MiscAttrs.cpp
  src/Node.cpp
  src/NodeAdd.cpp
  src/NodeChange.cpp
  src/NodeContainer.cpp
  src/NodeDelete.cpp
  src/NodeFind.cpp
  src/NodeMemento.cpp
  src/NodeStats.cpp
  src/NodeTime.cpp
  src/NodeTreeVisitor.cpp
  src/ResolveExternsVisitor.cpp
  src/ServerState.cpp
  src/Signal.cpp
  src/Submittable.cpp
  src/Suite.cpp
  src/SuiteChanged.cpp
  src/System.cpp
  src/Task.cpp
  src/TaskScriptGenerator.cpp
  parser/src/AutoArchiveParser.cpp
  parser/src/AutoCancelParser.cpp
  parser/src/AutoRestoreParser.cpp
  parser/src/CalendarParser.cpp
  parser/src/ClockParser.cpp
  parser/src/CronParser.cpp
  parser/src/DateParser.cpp
  parser/src/DayParser.cpp
  parser/src/DefsParser.cpp
  parser/src/DefsStateParser.cpp
  parser/src/DefsStatusParser.cpp
  parser/src/DefsStructureParser.cpp
  parser/src/EventParser.cpp
  parser/src/ExternParser.cpp
  parser/src/GenericParser.cpp
  parser/src/InlimitParser.cpp
  parser/src/LabelParser.cpp
  parser/src/LateParser.cpp
  parser/src/LimitParser.cpp
  parser/src/MeterParser.cpp
  parser/src/Parser.cpp
  parser/src/QueueParser.cpp
  parser/src/RepeatParser.cpp
  parser/src/TimeParser.cpp
  parser/src/TodayParser.cpp
  parser/src/TriggerParser.cpp
  parser/src/VariableParser.cpp
  parser/src/VerifyParser.cpp
  parser/src/ZombieAttrParser.cpp
)

ecbuild_add_library(
  TARGET
    node
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
    parser/src
  PUBLIC_LIBS
    nodeattr
)
target_clangformat(node)


set(test_srcs
  # Headers
  test/MyDefsFixture.hpp
  # Sources
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
  test/TestMissNextTimeSlot.cpp
  test/TestMovePeer.cpp
  test/TestNodeBeginReque.cpp
  test/TestNodeState.cpp
  test/TestNode_main.cpp # test entry point
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
  test/Test_ECFLOW-195.cpp
  test/Test_ECFLOW-247.cpp
  test/Test_ECFLOW-417.cpp
)

ecbuild_add_test(
  TARGET
    u_anode
  SOURCES
    ${test_srcs}
  LIBS
    node
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  TEST_DEPENDS
    u_anattr
)
target_clangformat(u_anode
  CONDITION ENABLE_TESTS)


set(test_srcs
  # Headers
  parser/test/PersistHelper.hpp
  # Sources
  parser/test/PersistHelper.cpp
  parser/test/TestAutoAddExterns.cpp
  parser/test/TestDefsStructurePersistAndReload.cpp
  parser/test/TestMementoPersistAndReload.cpp
  parser/test/TestMigration.cpp
  parser/test/TestParser.cpp
  parser/test/TestParser_main.cpp # test entry point
  parser/test/TestVariableParsing.cpp
)
ecbuild_add_test(
  TARGET
    u_aparser
  SOURCES
    ${test_srcs}
  INCLUDES
    test
  LIBS
    node
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  TEST_DEPENDS
    u_anode
)
target_clangformat(u_aparser
  CONDITION ENABLE_TESTS
)

if (ENABLE_ALL_TESTS)
  set(test_srcs
    test/TestSingleExprParse.cpp
    test/TestSingleExprParse_main.cpp # test entry point
  )

  ecbuild_add_test(
    TARGET
      u_anode_stest
    SOURCES
      ${test_srcs}
    LIBS
      node
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    TEST_DEPENDS
      u_anattr
  )
  target_clangformat(u_anode_stest
    CONDITION ENABLE_TESTS
  )


  set(test_srcs
    # Headers
    parser/test/PersistHelper.hpp
    # Sources
    parser/test/PersistHelper.cpp
    parser/test/TestParserPerformance_main.cpp # test entry point
    parser/test/TestSingleDefsFile.cpp
  )

  ecbuild_add_test(
    TARGET
      perf_aparser
    SOURCES
      ${test_srcs}
    LIBS
      node
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      Boost::timer
  )
  target_clangformat(perf_aparser
    CONDITION ENABLE_TESTS
  )


  set(test_srcs
    # Headers
    parser/test/PersistHelper.hpp
    # Sources
    parser/test/ParseTimer.cpp
    parser/test/PersistHelper.cpp
  )

  # The following is not technically a test (as it makes no checks),
  # but a tool to measure the time it takes to parse 'mega.def' file
  ecbuild_add_test(
    TARGET
      perf_aparser_timer
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
    SOURCES
      ${test_srcs}
    LIBS
      node
      Boost::boost # Boost header-only libraries must be available
      Boost::timer
  )
  target_clangformat(perf_aparser_timer
    CONDITION ENABLE_TESTS
  )

  # The following is not technically a test (as it makes no checks),
  # but a tool to parse 'mega.def' file
  set(test_srcs
    parser/test/ParseOnly.cpp
  )
  ecbuild_add_test(
    TARGET
      perf_aparser_only
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/test/data/single_defs/mega.def
    SOURCES
      ${test_srcs}
    LIBS
      node
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  )
  target_clangformat(perf_aparser_only
    CONDITION ENABLE_TESTS
  )

endif()
