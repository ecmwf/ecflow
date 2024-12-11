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
)
target_clangformat(test_harness.client
  CONDITION ENABLE_TESTS
)
