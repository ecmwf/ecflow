# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# ecflow_services
# --------------------------------------------------

add_subdirectory(test/harness)

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
    test/aviso/TestAvisoBackend.cpp
    test/aviso/TestAvisoV2.cpp
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

# u_service_aviso_v2
# --------------------------------------------------

ecflow_add_test(
  TARGET
    u_service_aviso_v2
  LABELS
    unit nightly fast
  SOURCES
    # Headers -- utilities
    test/TestContentProvider.hpp
    # Sources -- utilities
    test/TestContentProvider.cpp
    # Sources -- the Aviso v2 backend (only compiled into the server)
    src/ecflow/service/aviso/v2/AvisoV2Backend.hpp
    src/ecflow/service/aviso/v2/AvisoV2Backend.cpp
    # Sources
    test/aviso/v2/TestAvisoV2_main.cpp # test entry point
    test/aviso/v2/TestAvisoV2Backend.cpp
    test/aviso/v2/TestAvisoV2Integration.cpp
  INCLUDES
    test
  LIBS
    ecflow_all
    aviso::ffi
    test_scaffold
    test_harness.aviso
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
  ENVIRONMENT
    # The local Aviso server listens on a port reserved from this base (see .ai/rules/ecflow-testing.md)
    ECF_TEST_AVISO_PORT_BASE=18000
  CONDITION
    ENABLE_TESTS AND ENABLE_AVISO
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
