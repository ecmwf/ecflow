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

set(srcs
  # Headers
  src/ecflow/service/aviso/Aviso.hpp
  src/ecflow/service/aviso/AvisoService.hpp
  src/ecflow/service/aviso/etcd/Address.hpp
  src/ecflow/service/aviso/etcd/Client.hpp
  src/ecflow/service/aviso/etcd/Range.hpp
  src/ecflow/service/executor/PeriodicTaskExecutor.hpp
  src/ecflow/service/mirror/MirrorService.hpp
  src/ecflow/service/Log.hpp
  src/ecflow/service/Registry.hpp
  # Sources
  src/ecflow/service/aviso/Aviso.cpp
  src/ecflow/service/aviso/AvisoService.cpp
  src/ecflow/service/aviso/etcd/Range.cpp
  src/ecflow/service/aviso/etcd/Client.cpp
  src/ecflow/service/mirror/MirrorService.cpp
  src/ecflow/service/Log.hpp
  src/ecflow/service/Registry.cpp
)

ecbuild_add_library(
  TARGET ecflow_service
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    base
    libclient
    Boost::boost
    nlohmann::json
    httplib::httplib
    Threads::Threads
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
)
target_clangformat(ecflow_service)

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
    ecflow_service
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
    ecflow_service
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
)
target_clangformat(u_aviso
  CONDITION ENABLE_TESTS
)
