# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# FindHttplib
# -----------
#

#
# Find HTTP library include dirs
#
# Use this module by invoking find_package with the form:
#
#   find_package(Httplib
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# This module finds headers and libraries, specifying the following variables:
#
#   HTTPLIB_FOUND            - True if library is found
#   HTTPLIB_INCLUDE_DIRS     - Include directories to be used
#
# The following `IMPORTED` targets are also defined:
#
#   httplib::httplib         - Generic target for the library
#
#

if (NOT DEFINED HTTPLIB_DIR)
  message(FATAL_ERROR "Unable to find Httplib. Please provide HTTPLIB_LIB property.")
endif ()

#
# -----------------------------------------------------------------------------
# Search for include DIRs
# -----------------------------------------------------------------------------

find_path(HTTPLIB_INCLUDE_DIRS
  NAMES httplib.h
  PATHS ${HTTPLIB_DIR}/include)

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED and QUIET  parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Httplib
  REQUIRED_VARS
  HTTPLIB_INCLUDE_DIRS)

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(NAME "httplib")

add_library(${NAME} INTERFACE IMPORTED GLOBAL)

set_target_properties(${NAME}
  PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${HTTPLIB_INCLUDE_DIRS}")

add_library(httplib::httplib ALIAS httplib)
