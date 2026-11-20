# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

if (NOT ENABLE_SERVER)
  return() # Do not proceed if server is not enabled
endif ()

# The privileged ownership tests: they only run in the privileged test image (see docker/README.md), and report
# themselves skipped (exit status 77) elsewhere. They are not part of the nightly tests.
ecflow_add_test(
  TARGET
    s_ownership
  LABELS
    integration privileged slow
  SOURCES
    # Test entry point
    TestOwnership_main.cpp
    # Helpers
    PrivilegedEnvironment.hpp
    # Test sources
    TestOwnership.Commands.cpp
    TestOwnership.Plug.cpp
    TestOwnership.Startup.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    ${ADDITIONAL_BOOST_COMPONENTS}
  DEPENDS
    ecflow_server
    ecflow_client
  ENVIRONMENT
    # The launched servers listen on ports reserved from this base (range 15000-15999)
    ECF_TEST_OWNERSHIP_PORT_BASE=15000
  TEST_PROPERTIES
    SKIP_RETURN_CODE 77
  CONDITION
    ENABLE_TESTS
  FORMAT
)
