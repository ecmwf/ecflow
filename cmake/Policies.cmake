# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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
