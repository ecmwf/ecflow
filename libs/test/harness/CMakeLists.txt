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
  ServerTestHarness.hpp
  TestFixture.hpp
  ZombieUtil.hpp
  # Sources
  ServerTestHarness.cpp
  TestFixture.cpp
  ZombieUtill.cpp
)

ecbuild_add_library(
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
)
target_clangformat(test_harness.test
  CONDITION ENABLE_TESTS
)
