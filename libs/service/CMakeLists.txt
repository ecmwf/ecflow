# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# ecflow_services
# --------------------------------------------------

# u_service_executor
# --------------------------------------------------

ecflow_add_test(
  TARGET
    u_service_executor
  LABELS
    unit nightly fast
  SOURCES
    # Sources
    test/executor/TestExecutor_main.cpp # test entry point
    test/executor/TestPeriodicTaskExecutor.cpp
  LIBS
    ecflow_all
    test_scaffold
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  CONDITION
    ENABLE_TESTS
  FORMAT
)

# u_service_auth
# --------------------------------------------------

ecflow_add_test(
  TARGET
    u_service_auth
  LABELS
    unit nightly fast
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
  CONDITION
    ENABLE_TESTS
  FORMAT
)

# u_service_aviso
# --------------------------------------------------

ecflow_add_test(
  TARGET
    u_service_aviso
  LABELS
    unit nightly fast
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
    $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
  DEFINITIONS
    $<$<BOOL:${ZLIB_FOUND}>:ECF_HTTP_COMPRESSION>
  CONDITION
    ENABLE_TESTS
  FORMAT
)

# u_service_mirror
# --------------------------------------------------

ecflow_add_test(
  TARGET
    u_service_mirror
  LABELS
    unit nightly fast
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
    $<$<BOOL:${ZLIB_FOUND}>:ZLIB::ZLIB>
  DEFINITIONS
    $<$<BOOL:${ZLIB_FOUND}>:ECF_HTTP_COMPRESSION>
  CONDITION
    ENABLE_TESTS
  FORMAT
)
