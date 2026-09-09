# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

if (ENABLE_SERVER)
  add_subdirectory(foolproof)
  add_subdirectory(overall)
endif ()
add_subdirectory(simulator)
