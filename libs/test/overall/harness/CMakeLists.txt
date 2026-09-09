# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set(srcs
  # Headers
  LocalServerLauncher.hpp
  ScratchDir.hpp
  ServerTestHarness.hpp
  TestFixture.hpp
  ZombieUtil.hpp
  # Sources
  LocalServerLauncher.cpp
  ServerTestHarness.cpp
  TestFixture.cpp
  ZombieUtill.cpp
)

ecflow_add_library(
  TARGET
    test_harness.test
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    ./
  PUBLIC_LIBS
    ecflow_all
    test_scaffold
    test_harness.base
    test_harness.client
  CONDITION
    ENABLE_TESTS
  FORMAT
)
