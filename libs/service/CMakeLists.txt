#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# ecflow_services
# --------------------------------------------------

set(srcs
  # Headers
  src/ecflow/service/Registry.hpp
  # Sources
  src/ecflow/service/Registry.cpp
)

ecbuild_add_library(
  TARGET ecflow_service
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    libaviso
    base
)
target_clangformat(ecflow_service)

# u_services
# --------------------------------------------------

set(test_srcs
  # Sources
  test/TestRegistry.cpp
  test/TestServices_main.cpp # test entry point
)

ecbuild_add_test(
  TARGET
    u_service
  LABELS
    unit
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_service
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_service
  CONDITION ENABLE_TESTS
)
