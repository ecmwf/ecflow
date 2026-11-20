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
# The kind of library is selected by the following (optional) variable:
#
#   AVISO_FFI_USE_STATIC_LIBS - Set to ON to use the static library (libaviso_ffi.a), instead of the shared library
#                               (default: OFF)
#
# The pkg-config tool itself is not used, as it is not available on every build host. The version is read directly
# from the pkg-config file shipped with the library (lib/pkgconfig/aviso_ffi.pc), or, in its absence, from the file
# name of the fully versioned shared library (e.g. libaviso_ffi.2.4.2.dylib, libaviso_ffi.so.2.4.2).
#
# The static library depends on system libraries, which are read from the Libs.private entry of the same
# pkg-config file, or, in its absence, set to the system libraries known to be required on the platform.
#
# This module defines the following variables:
#
#   AVISO_FFI_FOUND          - True if library is found
#   AVISO_FFI_VERSION        - Version of the library found
#   AVISO_FFI_INCLUDE_DIRS   - Include directories to be used (headers are included as <aviso_ffi/aviso.hpp>)
#   AVISO_FFI_LIBRARIES      - Libraries to be linked (including, for the static library, its system libraries)
#   AVISO_FFI_LIBRARY_DIR    - Directory containing the library (for the shared library, to be used as run path)
#   AVISO_FFI_IS_STATIC      - True if the library found is the static library
#
# The following `IMPORTED` targets are also defined:
#
#   aviso::ffi               - Generic target for the library (SHARED or STATIC, as selected)
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

if (AVISO_FFI_USE_STATIC_LIBS)
  set(_AVISO_FFI_LIBRARY_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}aviso_ffi${CMAKE_STATIC_LIBRARY_SUFFIX})
else ()
  set(_AVISO_FFI_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}aviso_ffi${CMAKE_SHARED_LIBRARY_SUFFIX})
endif ()

# Search again when the library found (kept in the CMake cache) is not of the kind selected
string(REPLACE "." "\\." _AVISO_FFI_STATIC_SUFFIX_REGEX "${CMAKE_STATIC_LIBRARY_SUFFIX}$")
if (AVISO_FFI_LIBRARY)
  if (AVISO_FFI_USE_STATIC_LIBS AND NOT AVISO_FFI_LIBRARY MATCHES "${_AVISO_FFI_STATIC_SUFFIX_REGEX}")
    unset(AVISO_FFI_LIBRARY CACHE)
  elseif (NOT AVISO_FFI_USE_STATIC_LIBS AND AVISO_FFI_LIBRARY MATCHES "${_AVISO_FFI_STATIC_SUFFIX_REGEX}")
    unset(AVISO_FFI_LIBRARY CACHE)
  endif ()
endif ()

find_library(AVISO_FFI_LIBRARY
  NAMES ${_AVISO_FFI_LIBRARY_NAME}
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
# Determine the system libraries required by the static library
# -----------------------------------------------------------------------------

set(_AVISO_FFI_SYSTEM_LIBRARIES "")
set(_AVISO_FFI_SYSTEM_LINK_OPTIONS "")

if (AVISO_FFI_LIBRARY AND AVISO_FFI_USE_STATIC_LIBS)
  set(_AVISO_FFI_PC_LIBS_PRIVATE "")
  if (EXISTS "${_AVISO_FFI_PC}")
    file(STRINGS "${_AVISO_FFI_PC}" _AVISO_FFI_PC_LIBS_PRIVATE REGEX "^Libs\\.private:")
    string(REGEX REPLACE "^Libs\\.private:[ \t]*" "" _AVISO_FFI_PC_LIBS_PRIVATE "${_AVISO_FFI_PC_LIBS_PRIVATE}")
  endif ()

  if (NOT _AVISO_FFI_PC_LIBS_PRIVATE)
    if (APPLE)
      set(_AVISO_FFI_PC_LIBS_PRIVATE "-framework Security -framework CoreFoundation -liconv")
    else ()
      set(_AVISO_FFI_PC_LIBS_PRIVATE "-lgcc_s -lutil -lrt -lpthread -lm -ldl")
    endif ()
  endif ()

  # Libraries (-l<name>) are linked by name, and frameworks (-framework <name>) as link options; the C library
  # (and the macOS System library) are always linked, and therefore skipped
  separate_arguments(_AVISO_FFI_PC_LIBS_PRIVATE UNIX_COMMAND "${_AVISO_FFI_PC_LIBS_PRIVATE}")
  set(_AVISO_FFI_EXPECT_FRAMEWORK FALSE)
  foreach (_AVISO_FFI_ITEM IN LISTS _AVISO_FFI_PC_LIBS_PRIVATE)
    if (_AVISO_FFI_EXPECT_FRAMEWORK)
      list(APPEND _AVISO_FFI_SYSTEM_LINK_OPTIONS "SHELL:-framework ${_AVISO_FFI_ITEM}")
      set(_AVISO_FFI_EXPECT_FRAMEWORK FALSE)
    elseif (_AVISO_FFI_ITEM STREQUAL "-framework")
      set(_AVISO_FFI_EXPECT_FRAMEWORK TRUE)
    elseif (_AVISO_FFI_ITEM MATCHES "^-l(.+)$")
      set(_AVISO_FFI_NAME "${CMAKE_MATCH_1}")
      if (NOT _AVISO_FFI_NAME MATCHES "^(c|System)$")
        list(APPEND _AVISO_FFI_SYSTEM_LIBRARIES "${_AVISO_FFI_NAME}")
      endif ()
    else ()
      list(APPEND _AVISO_FFI_SYSTEM_LINK_OPTIONS "${_AVISO_FFI_ITEM}")
    endif ()
  endforeach ()
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
    "Provide the location of an unpacked aviso-ffi release with AVISO_FFI_ROOT (including the static library, when ENABLE_STATIC_AVISO_LIBS=ON), or disable Aviso support with ENABLE_AVISO=OFF")

#
# -----------------------------------------------------------------------------
# Define library as exported targets
# -----------------------------------------------------------------------------

set(AVISO_FFI_FOUND ${AvisoFfi_FOUND})

if (AVISO_FFI_FOUND)
  set(AVISO_FFI_INCLUDE_DIRS ${AVISO_FFI_INCLUDE_DIR})

  if (AVISO_FFI_USE_STATIC_LIBS)
    set(AVISO_FFI_IS_STATIC TRUE)
    set(AVISO_FFI_LIBRARIES ${AVISO_FFI_LIBRARY} ${_AVISO_FFI_SYSTEM_LIBRARIES})
    foreach (_AVISO_FFI_OPTION IN LISTS _AVISO_FFI_SYSTEM_LINK_OPTIONS)
      string(REGEX REPLACE "^SHELL:" "" _AVISO_FFI_OPTION "${_AVISO_FFI_OPTION}")
      list(APPEND AVISO_FFI_LIBRARIES "${_AVISO_FFI_OPTION}")
    endforeach ()
  else ()
    set(AVISO_FFI_IS_STATIC FALSE)
    set(AVISO_FFI_LIBRARIES ${AVISO_FFI_LIBRARY})
  endif ()

  if (NOT TARGET aviso::ffi)
    if (AVISO_FFI_IS_STATIC)
      add_library(aviso::ffi STATIC IMPORTED GLOBAL)
      set_target_properties(aviso::ffi
        PROPERTIES
          IMPORTED_LOCATION "${AVISO_FFI_LIBRARY}"
          IMPORTED_LINK_INTERFACE_LANGUAGES "C"
          INTERFACE_INCLUDE_DIRECTORIES "${AVISO_FFI_INCLUDE_DIR}"
          INTERFACE_LINK_LIBRARIES "${_AVISO_FFI_SYSTEM_LIBRARIES}"
          INTERFACE_LINK_OPTIONS "${_AVISO_FFI_SYSTEM_LINK_OPTIONS}")
    else ()
      add_library(aviso::ffi SHARED IMPORTED GLOBAL)
      set_target_properties(aviso::ffi
        PROPERTIES
          IMPORTED_LOCATION "${AVISO_FFI_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${AVISO_FFI_INCLUDE_DIR}")
    endif ()
  endif ()
endif ()

mark_as_advanced(AVISO_FFI_INCLUDE_DIR AVISO_FFI_LIBRARY)
