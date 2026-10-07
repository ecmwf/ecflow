# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

ecflow_add_library(
  TARGET
    test_harness.aviso
  NOINSTALL
  TYPE
    STATIC
  SOURCES
    # Headers
    AvisoTestSupport.hpp
    LocalAvisoServer.hpp
    # Sources
    LocalAvisoServer.cpp
  PUBLIC_INCLUDES
    .
  PUBLIC_LIBS
    ecflow_all
  CONDITION
    ENABLE_TESTS AND ENABLE_AVISO
  FORMAT
)
