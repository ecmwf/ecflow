# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

ecflow_add_library(
  TARGET
    test_harness.client
  NOINSTALL
  TYPE
    STATIC
  SOURCES
    # Headers
    InvokeServer.hpp
    SCPort.hpp
    # Sources
    SCPort.cpp
  PUBLIC_INCLUDES
    .
  PUBLIC_LIBS
    ecflow_all
    test_harness.base
  CONDITION
    ENABLE_TESTS
  FORMAT
)
