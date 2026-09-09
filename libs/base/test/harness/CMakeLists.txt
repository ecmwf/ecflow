# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

ecflow_add_library(
  TARGET
    test_harness.base
  NOINSTALL
  TYPE STATIC
  SOURCES
    # Headers
    MockServer.hpp
    TestHelper.hpp
    # Sources
    MockServer.cpp
  PUBLIC_INCLUDES
    ./
  PUBLIC_LIBS
    ecflow_all
  CONDITION
    ENABLE_TESTS
  FORMAT
)
