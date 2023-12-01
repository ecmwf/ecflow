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
ecbuild_add_library(
  TARGET
    libsimu
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    node
    nodeattr
    core
)
target_clangformat(libsimu)

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
  test/TestTime.cpp
  test/TestToday.cpp
  test/TestUtil.cpp
)
ecbuild_add_test(
  TARGET
    c_csim
  LABELS
    integration nightly
  SOURCES
    ${test_srcs}
  LIBS
    libsimu
    Threads::Threads
  TEST_DEPENDS u_base
)
target_clangformat(c_csim CONDITION ENABLE_TESTS)

if (ENABLE_ALL_TESTS)

  ecbuild_add_test(
    TARGET
      c_csim_single
    LABELS
      integration nightly
    SOURCES
      test/TestSingleSimulator.cpp
      test/TestUtil.cpp
    LIBS
      libsimu
      Threads::Threads
    TEST_DEPENDS
      u_base
  )
  target_clangformat(c_csim_single CONDITION ENABLE_TESTS)

endif()
