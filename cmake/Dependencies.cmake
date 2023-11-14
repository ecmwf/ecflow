#
# Copyright 2023- ECMWF.
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
