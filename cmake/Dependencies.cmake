#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/3rdparty")

# =========================================================================================
# Threads
# =========================================================================================
find_package(Threads REQUIRED)

# =========================================================================================
# Cereal
# =========================================================================================
set(CEREAL_DIR "${DEPENDENCIES_DIR}/cereal")
find_package(Cereal REQUIRED)

# =========================================================================================
# Json
# =========================================================================================
set(JSON_DIR "${DEPENDENCIES_DIR}/json")
find_package(Json REQUIRED)

# =========================================================================================
# Httplib
# =========================================================================================
set(HTTPLIB_DIR "${DEPENDENCIES_DIR}/cpp-httplib")
find_package(Httplib REQUIRED)

# =========================================================================================
# zlib
# =========================================================================================
if (ENABLE_HTTP_COMPRESSION)
  find_package(ZLIB)
  if (NOT ZLIB_FOUND)
    message(FATAL_ERROR "HTTP compression support requested, but zlib was not found")
  endif ()
endif ()

# =========================================================================================
# Crypt
# =========================================================================================
ecbuild_info( "Locating Crypt" )

find_package(Crypt)

ecbuild_info( "Crypt details:" )
ecbuild_info( " * Crypt_FOUND        : ${Crypt_FOUND}" )
ecbuild_info( " * Crypt_INCLUDE_DIRS : ${Crypt_INCLUDE_DIRS}" )
ecbuild_info( " * Crypt_LIBRARIES    : ${Crypt_LIBRARIES}" )
