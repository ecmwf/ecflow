# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "ecflow")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}")

if (CUSTOM_DEBIAN_PACKAGE_VERSION)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CUSTOM_DEBIAN_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
  ecbuild_info("Custom Debian package version: ${CPACK_PACKAGE_FILE_NAME}")
else()
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
  ecbuild_info("Default Debian package version: ${CPACK_PACKAGE_FILE_NAME}")
endif()

# The runtime dependencies are derived from the packaged binaries by dpkg-shlibdeps, so that the package
# depends on the shared libraries the binaries actually link, whatever the distribution and the enabled
# components. CPack only runs dpkg-shlibdeps when the 'file' utility is available, which is therefore required.
find_program(FILE_EXECUTABLE file)
if(NOT FILE_EXECUTABLE)
  ecbuild_critical("The 'file' utility was not found - it is required to derive the Debian package's dependencies (dpkg-shlibdeps)")
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
ecbuild_info("Debian package dependencies: derived by dpkg-shlibdeps")

if(ENABLE_PYTHON)
  # The Python module is built for, and installed into, a specific Python version, which dpkg-shlibdeps
  # does not detect (a Python extension module does not link libpython)
  math(EXPR _python3_next_minor "${Python3_VERSION_MINOR} + 1")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
      "python3 (>= ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}), python3 (<< ${Python3_VERSION_MAJOR}.${_python3_next_minor})")
endif()

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "software@ecmwf.int")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

include(CPack)
