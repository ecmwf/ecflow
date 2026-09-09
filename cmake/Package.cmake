# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

if (ENABLE_DEBIAN_PACKAGE)
  message(STATUS "Configuring Package: Debian (.deb)")
  include(cmake/PackageDebian.cmake)
else()
  message(STATUS "Configuring Package: Generic (.tar.gz)")
  include(cmake/PackageGeneric.cmake)
endif()
