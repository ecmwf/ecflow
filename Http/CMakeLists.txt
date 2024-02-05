#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# cpp-httplib requires at least OpenSSL 1.1.1 for https support
# so if we have OpenSSL enabled and the version is less than this,
# we should disable SSL capabilities from the HTTP server

if (OPENSSL_FOUND)
  if (OPENSSL_VERSION VERSION_LESS 1.1.1)
    ecbuild_warn("HTTP_SERVER requires at least OpenSSL 1.1.1 for https support (have version ${OPENSSL_VERSION}) - disabling https in HTTP server")
    remove_definitions(-DECF_OPENSSL)
    set(DISABLED_SSL_IN_HTTP_SERVER 1)
  endif ()
endif ()

set(srcs
  # Headers
  src/ecflow/http/Api.hpp
  src/ecflow/http/ApiV1.hpp
  src/ecflow/http/ApiV1Impl.hpp
  src/ecflow/http/Base64.hpp
  src/ecflow/http/BasicAuth.hpp
  src/ecflow/http/Client.hpp
  src/ecflow/http/DefsStorage.hpp
  src/ecflow/http/HttpServer.hpp
  src/ecflow/http/HttpServerException.hpp
  src/ecflow/http/JSON.hpp
  src/ecflow/http/Options.hpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/http/TokenStorage.hpp>
  src/ecflow/http/TreeGeneration.hpp
  src/ecflow/http/TypeToJson.hpp
  # Sources
  src/ecflow/http/Api.cpp
  src/ecflow/http/ApiV1.cpp
  src/ecflow/http/ApiV1Impl.cpp
  src/ecflow/http/BasicAuth.cpp
  src/ecflow/http/Client.cpp
  src/ecflow/http/DefsStorage.cpp
  src/ecflow/http/HttpServer.cpp
  src/ecflow/http/JSON.cpp
  src/ecflow/http/Options.cpp
  src/ecflow/http/TypeToJson.cpp
  $<$<BOOL:${OPENSSL_FOUND}>:src/ecflow/http/TokenStorage.cpp>
)

ecbuild_add_library(
  TARGET libhttp
  NOINSTALL
  TYPE STATIC
  SOURCES
    ${srcs}
  PUBLIC_INCLUDES
    src
  PUBLIC_LIBS
    base
    libclient
    nlohmann::json
    httplib::httplib
)
target_clangformat(libhttp)

# ========================================================================
# EXE ecflow_http, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable(
  TARGET ecflow_http
  SOURCES
    src/ecflow/http/HttpMain.cpp
  LIBS
    libhttp
    libclient
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
)
target_clangformat(ecflow_http)

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_http
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

if (ENABLE_HTTP AND ENABLE_SERVER)

  if (OPENSSL_FOUND AND NOT DISABLED_SSL_IN_HTTP_SERVER)

    set(test_srcs
      # Headers
      test/Certificate.hpp
      test/InvokeServer.hpp
      test/TokenFile.hpp
      # Sources
      test/TestApiV1.cpp
      test/TestHttp_main.cpp # test entry point
    )

    ecbuild_add_test(
      TARGET
        s_http
      LABELS
        integration nightly
      SOURCES
        ${test_srcs}
      INCLUDES
        ../Base/test
      LIBS
        libhttp
        libclient
        libserver
        Boost::boost # Boost header-only libraries must be available (namely unit_test_framework)
        $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
      DEPENDS
        ecflow_server # the server is launched to support tests
        ecflow_http
      TEST_DEPENDS
        u_base
        s_client
    )
    target_clangformat(s_http
      CONDITION ENABLE_TESTS
    )
  else ()
    message(WARNING "SSL not enabled - will not run HTTP server tests")
  endif ()

endif ()

# ===================================================================
# install
# ===================================================================
install(TARGETS ecflow_http DESTINATION bin)
