#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# =========================================================================================
# Setup project-wide CMake Policies
# =========================================================================================

#
# CMake Policy CMP0144 (CMake >=3.27)
#
# OLD behaviour:
#   find_package(PackageName) used only case-preserved <PackageName>_ROOT variables
#
# NEW behaviour:
#   find_package(PackageName) uses upper-case <PACKAGENAME>_ROOT variables,
#     in addition to <PackageName>_ROOT variables.
#
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()
