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

# u_service_executor
# --------------------------------------------------

ecbuild_add_test(
  TARGET
    u_service_executor
  LABELS
    unit nightly
  SOURCES
    # Sources
    test/executor/TestExecutor_main.cpp # test entry point
    test/executor/TestPeriodicTaskExecutor.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_service_executor
  CONDITION ENABLE_TESTS
)

# u_service_auth
# --------------------------------------------------

ecbuild_add_test(
  TARGET
    u_service_auth
  LABELS
    unit nightly
  SOURCES
    # Headers -- utilities
    test/TestContentProvider.hpp
    # Sources -- utilities
    test/TestContentProvider.cpp
  # Sources
    test/auth/TestAuth_main.cpp # test entry point
    test/auth/TestAuth.cpp
  INCLUDES
    test
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::filesystem
)
target_clangformat(u_service_auth
  CONDITION ENABLE_TESTS
)

# u_service_aviso
# --------------------------------------------------

ecbuild_add_test(
  TARGET
    u_service_aviso
  LABELS
    unit nightly
  SOURCES
    # Headers -- utilities
    test/TestContentProvider.hpp
    # Sources -- utilities
    test/TestContentProvider.cpp
    # Sources
    test/aviso/TestAviso_main.cpp # test entry point
    test/aviso/TestAviso.cpp
    test/aviso/TestAvisoService.cpp
  INCLUDES
    test
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    Boost::filesystem
)
target_clangformat(u_service_aviso
  CONDITION ENABLE_TESTS
)

# u_service_mirror
# --------------------------------------------------

ecbuild_add_test(
  TARGET
    u_service_mirror
  LABELS
    unit nightly
  SOURCES
    # Sources
    test/mirror/TestMirror_main.cpp # test entry point
    test/mirror/TestMirror.cpp
    test/mirror/TestMirrorClient.cpp
    test/mirror/TestMirrorService.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_service_mirror
  CONDITION ENABLE_TESTS
)
