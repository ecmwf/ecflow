# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# FindAvisoFfi
# ------------
#

#
# Find the Aviso client library (libaviso_ffi), i.e. the C ABI of aviso-client and its header-only C++ facade
#
# Use this module by invoking find_package with the form:
#
#   find_package(AvisoFfi
#     [version]              # Minimum version, e.g. 2.4.2
#     [REQUIRED]             # Fail with error if library is not found
#   )
#
# The search is guided by the following (optional) hints:
#
#   AVISO_FFI_ROOT           - CMake or environment variable, pointing to the root of an unpacked aviso-ffi release
#                              (i.e. the directory containing include/aviso_ffi/ and lib/)
#
# The library is located without pkg-config, which is not available on every build host. The version is taken from
# the pkg-config file shipped with the library (lib/pkgconfig/aviso_ffi.pc), or, in its absence, from the file name
# of the fully versioned shared library (e.g. libaviso_ffi.2.4.2.dylib, libaviso_ffi.so.2.4.2).
#
# Only the shared library is considered.
#
# This module defines the following variables:
#
#   AVISO_FFI_FOUND          - True if library is found
#   AVISO_FFI_VERSION        - Version of the library found
#   AVISO_FFI_INCLUDE_DIRS   - Include directories to be used (headers are included as <aviso_ffi/aviso.hpp>)
#   AVISO_FFI_LIBRARIES      - Libraries to be linked
#   AVISO_FFI_LIBRARY_DIR    - Directory containing the shared library (to be used as run path)
#
# The following `IMPORTED` targets are also defined:
#
#   aviso::ffi               - Generic target for the library
#
#

#
# -----------------------------------------------------------------------------
# Search for include DIRs and library
# -----------------------------------------------------------------------------

set(_AVISO_FFI_HINTS ${AVISO_FFI_ROOT} $ENV{AVISO_FFI_ROOT})

find_path(AVISO_FFI_INCLUDE_DIR
  NAMES aviso_ffi/aviso.h aviso_ffi/aviso.hpp
  HINTS ${_AVISO_FFI_HINTS}
  PATH_SUFFIXES include)

find_library(AVISO_FFI_LIBRARY
  NAMES ${CMAKE_SHARED_LIBRARY_PREFIX}aviso_ffi${CMAKE_SHARED_LIBRARY_SUFFIX}
  HINTS ${_AVISO_FFI_HINTS}
  PATH_SUFFIXES lib lib64)

#
# -----------------------------------------------------------------------------
# Determine the version
# -----------------------------------------------------------------------------

if (AVISO_FFI_LIBRARY)
  get_filename_component(AVISO_FFI_LIBRARY_DIR "${AVISO_FFI_LIBRARY}" DIRECTORY)

  set(_AVISO_FFI_PC "${AVISO_FFI_LIBRARY_DIR}/pkgconfig/aviso_ffi.pc")
  if (EXISTS "${_AVISO_FFI_PC}")
    file(STRINGS "${_AVISO_FFI_PC}" _AVISO_FFI_PC_VERSION REGEX "^Version:")
    string(REGEX REPLACE "^Version:[ \t]*([0-9]+(\\.[0-9]+)*).*$" "\\1" AVISO_FFI_VERSION "${_AVISO_FFI_PC_VERSION}")
  endif ()

  if (NOT AVISO_FFI_VERSION)
    get_filename_component(_AVISO_FFI_REALPATH "${AVISO_FFI_LIBRARY}" REALPATH)
    get_filename_component(_AVISO_FFI_REALNAME "${_AVISO_FFI_REALPATH}" NAME)
    if (_AVISO_FFI_REALNAME MATCHES "([0-9]+\\.[0-9]+\\.[0-9]+)")
      set(AVISO_FFI_VERSION "${CMAKE_MATCH_1}")
    endif ()
  endif ()
endif ()

#
# -----------------------------------------------------------------------------
# Handle find_package() REQUIRED, QUIET and version parameters
# -----------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AvisoFfi
  REQUIRED_VARS
    AVISO_FFI_LIBRARY
    AVISO_FFI_INCLUDE_DIR
    AVISO_FFI_VERSION
  VERSION_VAR
    AVISO_FFI_VERSION
  REASON_FAILURE_MESSAGE
    "Provide the location of an unpacked aviso-ffi release with AVISO_FFI_ROOT, or disable Aviso support with ENABLE_AVISO=OFF")

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(AVISO_FFI_FOUND ${AvisoFfi_FOUND})

if (AVISO_FFI_FOUND)
  set(AVISO_FFI_INCLUDE_DIRS ${AVISO_FFI_INCLUDE_DIR})
  set(AVISO_FFI_LIBRARIES ${AVISO_FFI_LIBRARY})

  if (NOT TARGET aviso::ffi)
    add_library(aviso::ffi SHARED IMPORTED GLOBAL)
    set_target_properties(aviso::ffi
      PROPERTIES
        IMPORTED_LOCATION "${AVISO_FFI_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${AVISO_FFI_INCLUDE_DIR}")
  endif ()
endif ()

mark_as_advanced(AVISO_FFI_INCLUDE_DIR AVISO_FFI_LIBRARY)
