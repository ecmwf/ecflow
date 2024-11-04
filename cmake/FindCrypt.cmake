#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# FindCrypt
# ---------
#

#
# Find Crypt library and include dirs
#
# Use this module by invoking find_package with the form:
#
#   find_package(Crypt
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# This module finds headers and libraries, specifying the following variables:
#
#   Crypt_FOUND            - True if library is found
#   Crypt_INCLUDE_DIRS     - Include directories to be used
#   Crypt_DEFINITIONS      - Compiler flags to be used
#
# The following `IMPORTED` targets are also defined:
#
#   crypt::crypt          - Generic target for the Crypt library
#
#

#
# -----------------------------------------------------------------------------
# Search for include DIRs
# -----------------------------------------------------------------------------

find_path(Crypt_INCLUDE_DIRS
  NAMES unistd.h
  HINTS
    /usr/include)

find_library(Crypt_LIBRARIES
  NAMES libcrypt.so
  HINTS
    /usr/lib)

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED and QUIET  parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Crypt
  REQUIRED_VARS
    Crypt_INCLUDE_DIRS
    Crypt_LIBRARIES)

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(NAME "crypt")

add_library(${NAME} INTERFACE IMPORTED GLOBAL)

set_target_properties(${NAME}
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Crypt_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${Crypt_LIBRARIES}")

add_library(crypt::crypt ALIAS crypt)
