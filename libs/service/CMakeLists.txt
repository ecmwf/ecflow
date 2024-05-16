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

# u_services
# --------------------------------------------------

set(test_srcs
  # Sources
  test/TestPeriodicTaskExecutor.cpp
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
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_service
  CONDITION ENABLE_TESTS
)

# u_aviso
# --------------------------------------------------

set(test_srcs
  # Sources
  test/aviso/TestAviso.cpp
  test/aviso/TestAviso_main.cpp # test entry point
)

ecbuild_add_test(
  TARGET
    u_aviso
  LABELS
    integration
  SOURCES
    ${test_srcs}
  LIBS
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_aviso
  CONDITION ENABLE_TESTS
)
