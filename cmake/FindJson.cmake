#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# FindJson
# ----------
#

#
# Find JSON library include dirs
#
# Use this module by invoking find_package with the form:
#
#   find_package(Json
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# This module finds headers and libraries, specifying the following variables:
#
#   JSON_FOUND            - True if library is found
#   JSON_INCLUDE_DIRS     - Include directories to be used
#
# The following `IMPORTED` targets are also defined:
#
#   nlohmann::json        - Generic target for the library
#
#

if (JSON_DIR)
  message(STATUS "Locating JSON at ${JSON_DIR}")
else ()
  message(FATAL_ERROR "Unable to find JSON. Please provide JSON_DIR property.")
endif ()

#
# -----------------------------------------------------------------------------
# Search for include DIRs
# -----------------------------------------------------------------------------

find_path(JSON_INCLUDE_DIRS
  NAMES nlohmann/json.hpp
  PATHS ${JSON_DIR}/include)

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED and QUIET  parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Json
  REQUIRED_VARS
  JSON_INCLUDE_DIRS)

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(NAME "nlohmann_json")

add_library(${NAME} INTERFACE IMPORTED GLOBAL)

set_target_properties(${NAME}
  PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${JSON_INCLUDE_DIRS}")

add_library(nlohmann::json ALIAS nlohmann_json)
