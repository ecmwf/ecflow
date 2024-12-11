#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

ecbuild_add_library(
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
)
target_clangformat(test_harness.base
  CONDITION ENABLE_TESTS
)
