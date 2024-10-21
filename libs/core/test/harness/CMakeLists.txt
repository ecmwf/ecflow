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
    test_harness.core
  NOINSTALL
  TYPE INTERFACE
  SOURCES
    TestSerialisation.hpp
    TestNaming.hpp
  PUBLIC_INCLUDES
    ./
  PUBLIC_LIBS
    ecflow_all
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(test_harness.core
  CONDITION ENABLE_TESTS
)
