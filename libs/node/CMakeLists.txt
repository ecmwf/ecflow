#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/ecflow/node/AbstractObserver.hpp
  src/ecflow/node/Alias.hpp
  src/ecflow/node/Aspect.hpp
  src/ecflow/node/Attr.hpp
  src/ecflow/node/AutoRestoreAttr.hpp
  src/ecflow/node/AvisoAttr.hpp
  src/ecflow/node/ClientSuiteMgr.hpp
  src/ecflow/node/ClientSuites.hpp
  src/ecflow/node/CmdContext.hpp
  src/ecflow/node/Defs.hpp
  src/ecflow/node/DefsDelta.hpp
  src/ecflow/node/DefsTreeVisitor.hpp
  src/ecflow/node/EcfFile.hpp
  src/ecflow/node/ExprAst.hpp
  src/ecflow/node/ExprAstVisitor.hpp
  src/ecflow/node/ExprDuplicate.hpp
  src/ecflow/node/ExprParser.hpp
  src/ecflow/node/Expression.hpp
  src/ecflow/node/Family.hpp
  src/ecflow/node/Flag.hpp
  src/ecflow/node/InLimit.hpp
  src/ecflow/node/InLimitMgr.hpp
  src/ecflow/node/JobCreationCtrl.hpp
  src/ecflow/node/JobProfiler.hpp
  src/ecflow/node/Jobs.hpp
  src/ecflow/node/JobsParam.hpp
  src/ecflow/node/Limit.hpp
  src/ecflow/node/LimitFwd.hpp
  src/ecflow/node/Memento.hpp
  src/ecflow/node/MiscAttrs.hpp
  src/ecflow/node/Node.hpp
  src/ecflow/node/NodeContainer.hpp
  src/ecflow/node/NodeFwd.hpp
  src/ecflow/node/NodeState.hpp
  src/ecflow/node/NodeStats.hpp
  src/ecflow/node/NodeTreeVisitor.hpp
  src/ecflow/node/ResolveExternsVisitor.hpp
  src/ecflow/node/ServerState.hpp
  src/ecflow/node/Signal.hpp
  src/ecflow/node/Submittable.hpp
  src/ecflow/node/Suite.hpp
  src/ecflow/node/SuiteChanged.hpp
  src/ecflow/node/System.hpp
  src/ecflow/node/Task.hpp
  src/ecflow/node/TaskScriptGenerator.hpp
  src/ecflow/node/move_peer.hpp
  src/ecflow/node/formatter/AvisoFormatter.hpp
  src/ecflow/node/formatter/Format.hpp
  src/ecflow/node/formatter/Formatter.hpp
  src/ecflow/node/parser/AutoArchiveParser.hpp
  src/ecflow/node/parser/AutoCancelParser.hpp
  src/ecflow/node/parser/AutoRestoreParser.hpp
  src/ecflow/node/parser/AvisoParser.hpp
  src/ecflow/node/parser/CalendarParser.hpp
  src/ecflow/node/parser/ClockParser.hpp
  src/ecflow/node/parser/CronParser.hpp
  src/ecflow/node/parser/DateParser.hpp
  src/ecflow/node/parser/DayParser.hpp
  src/ecflow/node/parser/DefsParser.hpp
  src/ecflow/node/parser/DefsStateParser.hpp
  src/ecflow/node/parser/DefsStatusParser.hpp
  src/ecflow/node/parser/DefsStructureParser.hpp
  src/ecflow/node/parser/EventParser.hpp
  src/ecflow/node/parser/ExternParser.hpp
  src/ecflow/node/parser/GenericParser.hpp
  src/ecflow/node/parser/InlimitParser.hpp
  src/ecflow/node/parser/LabelParser.hpp
  src/ecflow/node/parser/LateParser.hpp
  src/ecflow/node/parser/LimitParser.hpp
  src/ecflow/node/parser/MeterParser.hpp
  src/ecflow/node/parser/Parser.hpp
  src/ecflow/node/parser/QueueParser.hpp
  src/ecflow/node/parser/RepeatParser.hpp
  src/ecflow/node/parser/TimeParser.hpp
  src/ecflow/node/parser/TodayParser.hpp
  src/ecflow/node/parser/TriggerParser.hpp
  src/ecflow/node/parser/VariableParser.hpp
  src/ecflow/node/parser/VerifyParser.hpp
  src/ecflow/node/parser/ZombieAttrParser.hpp
  # Sources
  src/ecflow/node/Alias.cpp
  src/ecflow/node/Attr.cpp
  src/ecflow/node/AutoRestoreAttr.cpp
  src/ecflow/node/AvisoAttr.cpp
  src/ecflow/node/ClientSuiteMgr.cpp
  src/ecflow/node/ClientSuites.cpp
  src/ecflow/node/CmdContext.cpp
  src/ecflow/node/Defs.cpp
  src/ecflow/node/DefsDelta.cpp
  src/ecflow/node/EcfFile.cpp
  src/ecflow/node/ExprAst.cpp
  src/ecflow/node/ExprAstVisitor.cpp
  src/ecflow/node/ExprDuplicate.cpp
  src/ecflow/node/ExprParser.cpp
  src/ecflow/node/Expression.cpp
  src/ecflow/node/Family.cpp
  src/ecflow/node/Flag.cpp
  src/ecflow/node/InLimit.cpp
  src/ecflow/node/InLimitMgr.cpp
  src/ecflow/node/JobCreationCtrl.cpp
  src/ecflow/node/JobProfiler.cpp
  src/ecflow/node/Jobs.cpp
  src/ecflow/node/JobsParam.cpp
  src/ecflow/node/Limit.cpp
  src/ecflow/node/Memento.cpp
  src/ecflow/node/MiscAttrs.cpp
  src/ecflow/node/Node.cpp
  src/ecflow/node/NodeAdd.cpp
  src/ecflow/node/NodeChange.cpp
  src/ecflow/node/NodeContainer.cpp
  src/ecflow/node/NodeDelete.cpp
  src/ecflow/node/NodeFind.cpp
  src/ecflow/node/NodeMemento.cpp
  src/ecflow/node/NodeStats.cpp
  src/ecflow/node/NodeTime.cpp
  src/ecflow/node/NodeTreeVisitor.cpp
  src/ecflow/node/ResolveExternsVisitor.cpp
  src/ecflow/node/ServerState.cpp
  src/ecflow/node/Signal.cpp
  src/ecflow/node/Submittable.cpp
  src/ecflow/node/Suite.cpp
  src/ecflow/node/SuiteChanged.cpp
  src/ecflow/node/System.cpp
  src/ecflow/node/Task.cpp
  src/ecflow/node/TaskScriptGenerator.cpp
  src/ecflow/node/parser/AutoArchiveParser.cpp
  src/ecflow/node/parser/AutoCancelParser.cpp
  src/ecflow/node/parser/AutoRestoreParser.cpp
  src/ecflow/node/parser/AvisoParser.cpp
  src/ecflow/node/parser/CalendarParser.cpp
  src/ecflow/node/parser/ClockParser.cpp
  src/ecflow/node/parser/CronParser.cpp
  src/ecflow/node/parser/DateParser.cpp
  src/ecflow/node/parser/DayParser.cpp
  src/ecflow/node/parser/DefsParser.cpp
  src/ecflow/node/parser/DefsStateParser.cpp
  src/ecflow/node/parser/DefsStatusParser.cpp
  src/ecflow/node/parser/DefsStructureParser.cpp
  src/ecflow/node/parser/EventParser.cpp
  src/ecflow/node/parser/ExternParser.cpp
  src/ecflow/node/parser/GenericParser.cpp
  src/ecflow/node/parser/InlimitParser.cpp
  src/ecflow/node/parser/LabelParser.cpp
  src/ecflow/node/parser/LateParser.cpp
  src/ecflow/node/parser/LimitParser.cpp
  src/ecflow/node/parser/MeterParser.cpp
  src/ecflow/node/parser/Parser.cpp
  src/ecflow/node/parser/QueueParser.cpp
  src/ecflow/node/parser/RepeatParser.cpp
  src/ecflow/node/parser/TimeParser.cpp
  src/ecflow/node/parser/TodayParser.cpp
  src/ecflow/node/parser/TriggerParser.cpp
  src/ecflow/node/parser/VariableParser.cpp
  src/ecflow/node/parser/VerifyParser.cpp
  src/ecflow/node/parser/ZombieAttrParser.cpp
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
  PUBLIC_LIBS
    attributes
    ecflow_service
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
    test_support
    node
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
  test/parser/TestDefsStructurePersistAndReload.cpp
  test/parser/TestMementoPersistAndReload.cpp
  test/parser/TestMigration.cpp
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
    node
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
      node
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
      node
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
      Boost::timer
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
      node
      Boost::boost # Boost header-only libraries must be available
      Boost::timer
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
      node
      Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  )
  target_clangformat(p_parser_only
    CONDITION ENABLE_TESTS
  )

endif()
