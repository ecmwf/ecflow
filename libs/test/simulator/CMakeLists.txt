# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set(srcs
  # Headers
  src/ecflow/simulator/Analyser.hpp
  src/ecflow/simulator/AstAnalyserVisitor.hpp
  src/ecflow/simulator/DefsAnalyserVisitor.hpp
  src/ecflow/simulator/FlatAnalyserVisitor.hpp
  src/ecflow/simulator/Simulator.hpp
  src/ecflow/simulator/SimulatorVisitor.hpp
  # Sources
  src/ecflow/simulator/Analyser.cpp
  src/ecflow/simulator/AstAnalyserVisitor.cpp
  src/ecflow/simulator/DefsAnalyserVisitor.cpp
  src/ecflow/simulator/FlatAnalyserVisitor.cpp
  src/ecflow/simulator/Simulator.cpp
  src/ecflow/simulator/SimulatorVisitor.cpp
)
ecflow_add_library(
  TARGET
    libsimulator
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    ecflow_all
  FORMAT
)

set(test_srcs
  # Headers
  test/TestUtil.hpp
  # Sources
  test/TestAnalysis.cpp
  test/TestAutoArchive.cpp
  test/TestAutoCancel.cpp
  test/TestAutoCancel.cpp
  test/TestAutoRestore.cpp
  test/TestMeter.cpp
  test/TestRepeat.cpp
  test/TestSimulator.cpp
  test/TestSimulator_main.cpp # test entry point
  test/TestTime.cpp
  test/TestToday.cpp
  test/TestUtil.cpp
)
ecflow_add_test(
  TARGET
    s_simulator
  LABELS
    integration nightly fast
  SOURCES
    ${test_srcs}
  LIBS
    libsimulator
    Threads::Threads
  TEST_DEPENDS
    u_base
  CONDITION
    ENABLE_TESTS
  FORMAT
)

if (ENABLE_ALL_TESTS)

  ecflow_add_test(
    TARGET
      s_simulator_single
    LABELS
      integration nightly fast
    SOURCES
      test/TestSingleSimulator.cpp
      test/TestSingleSimulator_main.cpp # test entry point
      test/TestUtil.cpp
    LIBS
      libsimulator
      Threads::Threads
    TEST_DEPENDS
      u_base
    CONDITION
      ENABLE_TESTS
    FORMAT
  )

endif()
