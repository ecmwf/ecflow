#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# ==============================================================================
# ECFLOW UDP server

set(SERVER_TARGET ecflow_udp)

set(${SERVER_TARGET}_srcs
  # Sources
  src/ecflow/udp/UDPServerMain.cpp
)

ecbuild_add_executable(
  TARGET ${SERVER_TARGET}
  SOURCES ${${SERVER_TARGET}_srcs}
  INCLUDES
    src
  LIBS
    ecflow_all
)
set_target_properties(${SERVER_TARGET} PROPERTIES
  INSTALL_RPATH ""
)
target_clangformat(${SERVER_TARGET})


# ==============================================================================
# ECFLOW UDP Client

set(CLIENT_TARGET ecflow_udp_client)

set(${CLIENT_TARGET}_srcs
  # HEADERS
  src/ecflow/udp/UDPClient.hpp
  # SOURCES
  src/ecflow/udp/UDPClientMain.cpp
)

ecbuild_add_executable(
  TARGET ${CLIENT_TARGET}
  SOURCES ${${CLIENT_TARGET}_srcs}
  INCLUDES
    src
  LIBS
    pthread
    Boost::boost
    Boost::filesystem
)
set_target_properties(${CLIENT_TARGET} PROPERTIES
  INSTALL_RPATH ""
)

target_clangformat(${CLIENT_TARGET})

# ==============================================================================
# ECFLOW UDP Test(s)

set(TEST_TARGET s_udp)

list(APPEND ${TEST_TARGET}_srcs
  # HEADERS
  test/TestSupport.hpp
  # SOURCES
  test/TestUDP_main.cpp
  test/TestUDPServer.cpp
)

ecbuild_add_test(
  TARGET
    ${TEST_TARGET}
  LABELS
    integration nightly
  SOURCES
    ${${TEST_TARGET}_srcs}
  LIBS
    ${LIB_TARGET}
    ecflow_all
    Boost::boost
    Boost::unit_test_framework
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  DEPENDS
    ecflow_server # the server is launched to support tests
    ${SERVER_TARGET}
  DEFINITIONS
    ${BOOST_TEST_DYN_LINK}
  INCLUDES
    src
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/test_sandbox
  ENVIRONMENT
    "PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}"
)

target_clangformat(${TEST_TARGET} CONDITION ENABLE_TESTS)

file(COPY test/data/reference.def DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_sandbox/data)
