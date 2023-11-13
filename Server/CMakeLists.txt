#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(srcs
  # Headers
  src/BaseServer.hpp
  src/CheckPtSaver.hpp
  src/NodeTreeTraverser.hpp
  src/Server.hpp
  src/ServerEnvironment.hpp
  src/ServerOptions.hpp
  src/SslServer.hpp
  src/SslTcpServer.hpp
  src/TcpBaseServer.hpp
  src/TcpServer.hpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslServer.hpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslTcpServer.hpp>
  # Sources
  src/BaseServer.cpp
  src/CheckPtSaver.cpp
  src/NodeTreeTraverser.cpp
  src/Server.cpp
  src/ServerEnvironment.cpp
  src/ServerOptions.cpp
  src/TcpBaseServer.cpp
  src/TcpServer.cpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslServer.cpp>
  $<$<BOOL:${OPENSSL_FOUND}>:src/SslTcpServer.cpp>
)

ecbuild_add_library(
  TARGET
    libserver
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    base
    Boost::boost # Boost header-only libraries must be available (namely asio)
)
target_clangformat(libserver)

# =======================================================
# EXE ecflow_server, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable(
  TARGET
    ecflow_server
  SOURCES
    src/ServerMain.cpp
  LIBS
    libserver
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
)
target_clangformat(ecflow_server)

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_server
  PROPERTIES
    INSTALL_RPATH ""
)

# use, i.e. don't skip the full RPATH for the build tree
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

# the RPATH to be used when installing
#SET(CMAKE_INSTALL_RPATH "")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

                          
# ================================================================================                         
# TEST
ecbuild_add_test(
  TARGET
    u_server
  INCLUDES
    src
  SOURCES
    test/TestServerEnvironment.cpp
    test/TestServer_main.cpp # test entry point
    test/TestServer1.cpp
  LIBS
    libserver
    Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
    Threads::Threads
  TEST_DEPENDS
    u_base
)

target_clangformat(u_server CONDITION ENABLE_TESTS)

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_server DESTINATION bin)
