#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

if (NOT ENABLE_SERVER)
  return() # Do not proceed if server is not enabled
endif ()

ecbuild_add_test(
  TARGET
    s_foolproof
  LABELS
    integration
    nightly
  SOURCES
    # Test entry point
    TestFoolproof_main.cpp # test entry point
    # Test sources
    TestFoolproof.Basic.cpp
    TestFoolproof.InLimit.cpp
    TestFoolproof.Mirror.cpp
    TestFoolproof.NodeExtern.cpp
    TestFoolproof.PasswordsAndWhiteLists.cpp
    TestFoolproof.Scaffold.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::unit_test_framework
    ${ADDITIONAL_BOOST_COMPONENTS}
  DEPENDS
    ecflow_server
    ecflow_client
  ARGS
    --random=1
)
target_clangformat(s_foolproof CONDITION ENABLE_TESTS)
