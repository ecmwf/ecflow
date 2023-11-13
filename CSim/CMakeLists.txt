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
  src/Analyser.hpp
  src/AstAnalyserVisitor.hpp
  src/DefsAnalyserVisitor.hpp
  src/FlatAnalyserVisitor.hpp
  src/Simulator.hpp
  src/SimulatorVisitor.hpp
  # Sources
  src/Analyser.cpp
  src/AstAnalyserVisitor.cpp
  src/DefsAnalyserVisitor.cpp
  src/FlatAnalyserVisitor.cpp
  src/Simulator.cpp
  src/SimulatorVisitor.cpp
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
