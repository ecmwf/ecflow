# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# FindCereal
# ----------
#

#
# Find Cereal library include dirs
#
# Use this module by invoking find_package with the form:
#
#   find_package(Cereal
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# This module finds headers and libraries, specifying the following variables:
#
#   Cereal_FOUND            - True if library is found
#   Cereal_INCLUDE_DIRS     - Include directories to be used
#   Cereal_DEFINITIONS      - Compiler flags to be used
#
# The following `IMPORTED` targets are also defined:
#
#   cereal::cereal          - Generic target for the libraryCereal
#
#

if (NOT DEFINED CEREAL_DIR)
  message(FATAL_ERROR "Unable to find Cereal. Please provide CEREAL_DIR property.")
endif ()

#
# -----------------------------------------------------------------------------
# Search for include DIRs
# -----------------------------------------------------------------------------

find_path(Cereal_INCLUDE_DIRS
  NAMES cereal/cereal.hpp
  PATHS ${CEREAL_DIR}/include)

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED and QUIET  parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cereal
  REQUIRED_VARS
  Cereal_INCLUDE_DIRS)

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(NAME "cereal")

add_library(${NAME} INTERFACE IMPORTED GLOBAL)

set_target_properties(${NAME}
  PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${Cereal_INCLUDE_DIRS}")

add_library(cereal::cereal ALIAS cereal)
