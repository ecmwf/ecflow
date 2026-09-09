# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# FindSphinx
# ----------
#

#
# Find Sphinx documentation utilities
#
# Use this module by invoking find_package with the form:
#
#   find_package(Sphinx
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# This module finds headers and libraries, specifying the following variables:
#
#   SPHINX_EXECUTABLE       - True if tool is found
#   SPHINX_EXECUTABLE       - Path to the Sphinx executable to be used
#

#
# -----------------------------------------------------------------------------
# Search for executable(s)
# -----------------------------------------------------------------------------

find_program(SPHINX_EXECUTABLE
  NAMES sphinx-build
  DOC "Path to the Sphinx executable"
)

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED and QUIET  parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sphinx
  REQUIRED_VARS
    SPHINX_EXECUTABLE)
