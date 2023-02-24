#
# Copyright 2009-2023 ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

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
