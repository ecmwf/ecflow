
# ==============================================================================
# ECFLOW UDP library

set(LIB_TARGET libecflow_udp)

set(${LIB_TARGET}_srcs
  # HEADERS
  src/ClientAPI.hpp
  src/RequestHandler.hpp
  src/Trace.hpp
  src/UDPServer.hpp
  src/UDPServerEnvironment.hpp
  src/UDPServerOptions.hpp
  # SOURCES
  src/ClientAPI.cpp
  src/RequestHandler.cpp
  src/Trace.cpp
  src/UDPServerEnvironment.cpp
  src/UDPServerOptions.cpp
)

ecbuild_add_library(
  TARGET ${LIB_TARGET}
  TYPE STATIC
  NOINSTALL
  SOURCES ${${LIB_TARGET}_srcs}
  PRIVATE_INCLUDES
    src
    ../Client/src
    ../json
  PUBLIC_LIBS
    libclient
    pthread
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Boost::boost
)
target_clangformat(${LIB_TARGET})

# ==============================================================================
# ECFLOW UDP server

set(SERVER_TARGET ecflow_udp)

set(${SERVER_TARGET}_srcs
  # SOURCES
  src/UDPServerMain.cpp
)

ecbuild_add_executable(
  TARGET ${SERVER_TARGET}
  SOURCES ${${SERVER_TARGET}_srcs}
  INCLUDES
    src
    ../ACore/src   # Needed only to #include "ecflow_version.h"
  LIBS
    ${LIB_TARGET}
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
  src/UDPClient.hpp
  # SOURCES
  src/UDPClientMain.cpp
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
  test/TestMain.cpp
  test/TestUDPServer.cpp
)

ecbuild_add_test(
  TARGET ${TEST_TARGET}
  SOURCES ${${TEST_TARGET}_srcs}
  LIBS
    ${LIB_TARGET}
    libclient
    Boost::boost
    Boost::unit_test_framework
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  DEPENDS
    ${SERVER_TARGET}
  DEFINITIONS
    ECFLOW_UDP_CLIENT_BASED_ON_BOOST_ASIO
  INCLUDES
    src
    ../json
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/test_sandbox
  ENVIRONMENT
    "PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}"
)

target_clangformat(${TEST_TARGET})

file(COPY test/data/reference.def DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_sandbox/data)
