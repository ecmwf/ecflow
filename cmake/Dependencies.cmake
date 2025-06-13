#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

set(DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/3rdparty")

# =========================================================================================
# Threads
# =========================================================================================
find_package(Threads REQUIRED)


# =========================================================================================
# Cereal
# =========================================================================================
ecbuild_info( "Locating Cereal" )

set(CEREAL_DIR "${DEPENDENCIES_DIR}/cereal")
find_package(Cereal REQUIRED)

ecbuild_info( "Cereal details:" )
ecbuild_info( " * Cereal_FOUND        : ${Cereal_FOUND}" )
ecbuild_info( " * Cereal_INCLUDE_DIRS : ${Cereal_INCLUDE_DIRS}" )

ecbuild_info( "Found Cereal at ${Cereal_INCLUDE_DIRS}" )


# =========================================================================================
# Json
# =========================================================================================
ecbuild_info( "Locating JSON" )

set(JSON_DIR "${DEPENDENCIES_DIR}/json")
find_package(Json REQUIRED)

ecbuild_info( "JSON details:" )
ecbuild_info( " * JSON_FOUND        : ${JSON_FOUND}" )
ecbuild_info( " * JSON_INCLUDE_DIRS : ${JSON_INCLUDE_DIRS}" )

ecbuild_info( "Found JSON at ${JSON_INCLUDE_DIRS}" )


# =========================================================================================
# Httplib
# =========================================================================================
ecbuild_info( "Locating Httplib" )

set(HTTPLIB_DIR "${DEPENDENCIES_DIR}/cpp-httplib")
find_package(Httplib REQUIRED)

ecbuild_info( "Httplib details:" )
ecbuild_info( " * HTTPLIB_FOUND        : ${HTTPLIB_FOUND}" )
ecbuild_info( " * HTTPLIB_INCLUDE_DIRS : ${HTTPLIB_INCLUDE_DIRS}" )

ecbuild_info( "Found Httplib at ${HTTPLIB_INCLUDE_DIRS}" )


# =========================================================================================
# zlib
# =========================================================================================
if (ENABLE_HTTP_COMPRESSION)
  ecbuild_info( "Locating ZLIB" )

  find_package(ZLIB REQUIRED)

  ecbuild_info("ZLIB details:")
  ecbuild_info(" * ZLIB_FOUND        : ${ZLIB_FOUND}")
  ecbuild_info(" * ZLIB_INCLUDE_DIRS : ${ZLIB_INCLUDE_DIRS}")
  ecbuild_info(" * ZLIB_LIBRARIES    : ${ZLIB_LIBRARIES}")

  ecbuild_info( "Found ZLIB at ${ZLIB_INCLUDE_DIRS}" )
endif ()


# =========================================================================================
# Python3
# =========================================================================================
if (ENABLE_PYTHON)

  ecbuild_info( "Locating Python3" )

  # The python must include the Development packages. As the headers in these packages is used by boost python.
  find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

  ecbuild_info( "Python3 details:" )
  ecbuild_info( " * Python3_FOUND             : ${Python3_FOUND}" )
  ecbuild_info( " * Python3_Interpreter_FOUND : ${Python3_Interpreter_FOUND}" )
  ecbuild_info( " * Python3_EXECUTABLE        : ${Python3_EXECUTABLE}" )
  if (Python3_Interpreter_FOUND)
    get_filename_component(Python3_EXECUTABLE_DIR "${Python3_EXECUTABLE}" DIRECTORY)
    ecbuild_info( " * Python3_EXECUTABLE_DIR    : ${Python3_EXECUTABLE_DIR}" )
  endif()
  ecbuild_info( " * Python3_STDLIB            : ${Python3_STDLIB} (Standard platform independent installation directory)" )
  ecbuild_info( " * Python3_STDARCH           : ${Python3_STDARCH} (Standard platform dependent installation directory)" )
  ecbuild_info( " * Python3_Development_FOUND : ${Python3_Development_FOUND}" )
  ecbuild_info( " * Python3_INCLUDE_DIRS      : ${Python3_INCLUDE_DIRS}" )
  ecbuild_info( " * Python3_LIBRARIES         : ${Python3_LIBRARIES}" )
  ecbuild_info( " * Python3_LIBRARY_DIRS      : ${Python3_LIBRARY_DIRS}" )
  ecbuild_info( " * Python3_VERSION           : ${Python3_VERSION}" )
  ecbuild_info( " * Python3_VERSION_MAJOR     : ${Python3_VERSION_MAJOR}" )
  ecbuild_info( " * Python3_VERSION_MINOR     : ${Python3_VERSION_MINOR}" )
  ecbuild_info( " * Python3_VERSION_PATCH     : ${Python3_VERSION_PATCH}" )

  # Set (deprecated) FindPython variables
  # These need to be available, as they are used by `ecbuild_add_test(... TYPE PYTHON ...)`
  set(PYTHONINTERP_FOUND "${Python3_Interpreter_FOUND}")
  set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")

  ecbuild_info( "Found Python3 at ${Python3_INCLUDE_DIRS}" )

endif()


# =========================================================================================
# Boost
# =========================================================================================

ecbuild_info( "Locating Boost" )

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
  ecbuild_info( "Using BoostConfig.cmake to find Boost (CMake >= 3.30)" )
  cmake_policy(SET CMP0167 NEW)
else()
  ecbuild_info( "Using FindBoost.cmake module to find Boost (CMake < 3.30)" )
endif()

# To use static boost python ensure that Boost_USE_STATIC_LIBS is set on.
# See: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html
if ( ENABLE_STATIC_BOOST_LIBS )
  set(Boost_USE_STATIC_LIBS ON)
  set(BOOST_TEST_DYN_LINK "")
  ecbuild_info( "Using STATIC boost libraries" )
else()
  set(Boost_USE_STATIC_LIBS OFF)
  set(BOOST_TEST_DYN_LINK "BOOST_TEST_DYN_LINK")
  ecbuild_info( "Using SHARED boost libraries : (i.e. defining ${BOOST_TEST_DYN_LINK})" )
endif()

set(Boost_USE_MULTITHREADED    ON)
set(Boost_NO_SYSTEM_PATHS      ON)
set(Boost_DETAILED_FAILURE_MSG ON)
set(Boost_ARCHITECTURE         "-x64") # from boost 1.69 layout=tagged adds libboost_system-mt-x64.a, https://gitlab.kitware.com/cmake/cmake/issues/18908

set(ECFLOW_BOOST_VERSION "1.66.0") # Boost 1.66.0 is the minimum version required (needed to support Rocky 8.6 on CI)

find_package( Boost ${ECFLOW_BOOST_VERSION} QUIET REQUIRED) # This initial step allows to get a hold of Boost_VERSION_STRING

if ( Boost_VERSION_STRING VERSION_LESS "1.69.0" )
  # Boost::system is header-only from 1.69.0
  list(APPEND _boost_needed_libs system )
endif ()

list(APPEND _boost_needed_libs timer chrono filesystem program_options date_time )

if (ENABLE_PYTHON)
  # The following is used to find Boost.python library, as the library name changes with python version
  if ( Boost_MINOR_VERSION GREATER 66 )
    # cmake 3.15
    # see: https://gitlab.kitware.com/cmake/cmake/issues/19656
    # INTERFACE_LIBRARY targets may only have whitelisted properties.
    set(_python_base_version "${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
  else()
    set(_python_base_version "${Python3_VERSION_MAJOR}")
  endif()
  set(ECFLOW_BOOST_PYTHON_COMPONENT "python${_python_base_version}")

  if ( Boost_MINOR_VERSION GREATER_EQUAL 86 )
    list(APPEND _boost_needed_libs process)
  endif()

  list(APPEND _boost_needed_libs ${ECFLOW_BOOST_PYTHON_COMPONENT})
endif()

if(HAVE_TESTS) # HAVE_TESTS is defined if ecbuild ENABLE_TESTS is set, (by default this is set)
  list(APPEND _boost_needed_libs unit_test_framework test_exec_monitor )
endif()

find_package( Boost ${ECFLOW_BOOST_VERSION} QUIET REQUIRED COMPONENTS ${_boost_needed_libs})

set(SELECTED_BOOST_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

ecbuild_info( " * Boost_FOUND                : ${Boost_FOUND}" )
ecbuild_info( " * Boost_NO_BOOST_CMAKE       : ${Boost_NO_BOOST_CMAKE}" )
ecbuild_info( " * Boost_USE_MULTITHREADED    : ${Boost_USE_MULTITHREADED}" )
ecbuild_info( " * Boost_NO_SYSTEM_PATHS      : ${Boost_NO_SYSTEM_PATHS}" )
ecbuild_info( " * Boost_DETAILED_FAILURE_MSG : ${Boost_DETAILED_FAILURE_MSG}" )
ecbuild_info( " * Boost_VERSION              : ${Boost_VERSION}" )
ecbuild_info( " * Boost_MAJOR_VERSION        : ${Boost_MAJOR_VERSION}" )
ecbuild_info( " * Boost_MINOR_VERSION        : ${Boost_MINOR_VERSION}" )
ecbuild_info( " * Boost_SUBMINOR_VERSION     : ${Boost_SUBMINOR_VERSION}" )
ecbuild_info( " * Boost_INCLUDE_DIRS         : ${Boost_INCLUDE_DIRS}" )
ecbuild_info( " * Boost_LIBRARY_DIR_RELEASE  : ${Boost_LIBRARY_DIR_RELEASE}" )
ecbuild_info( " * Boost Components" )
foreach(_lib ${_boost_needed_libs})
  string(TOUPPER "${_lib}" _lib_upper)

  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    ecbuild_info( "   * ${_lib}, found ${Boost_${_lib_upper}_LIBRARY} @ ${Boost_${_lib_upper}_LIBRARY_RELEASE}" )
  else()
    ecbuild_info( "   * ${_lib}, found ${Boost_${_lib_upper}_LIBRARY_RELEASE}" )
  endif()
endforeach()

ecbuild_info( "Found Boost at ${Boost_INCLUDE_DIR}" )
if (ENABLE_PYTHON)
  ecbuild_info( "Found Boost.Python at ${Boost_PYTHON${_python_base_version}_LIBRARY_RELEASE}" )
endif()


# =========================================================================================
# OpenSSL
# =========================================================================================

if (ENABLE_SSL)
  ecbuild_info( "Locating OpenSSL" )

  find_package(OpenSSL REQUIRED)

  add_definitions( -DECF_OPENSSL=1 )

  ecbuild_info( "OpenSSL details:" )
  ecbuild_info( " * OPENSSL_FOUND       : ${OpenSSL_FOUND}" )
  ecbuild_info( " * OPENSSL_INCLUDE_DIR : ${OPENSSL_INCLUDE_DIR}" )
  ecbuild_info( " * OpenSSL_LIBRARIES   : ${OPENSSL_LIBRARIES}" )
  ecbuild_info( " * OPENSSL_VERSION     : ${OPENSSL_VERSION}" )

  ecbuild_info( "Found OpenSSL at ${OPENSSL_INCLUDE_DIR}" )
endif()


# =========================================================================================
# Crypt
# =========================================================================================
ecbuild_info( "Locating Crypt" )

find_package(Crypt)

ecbuild_info( "Crypt details:" )
ecbuild_info( " * Crypt_FOUND        : ${Crypt_FOUND}" )
ecbuild_info( " * Crypt_INCLUDE_DIRS : ${Crypt_INCLUDE_DIRS}" )
ecbuild_info( " * Crypt_LIBRARIES    : ${Crypt_LIBRARIES}" )

ecbuild_info( "Found Crypt at ${Crypt_INCLUDE_DIRS}" )


# =========================================================================================
# Dependency: Qt
# =========================================================================================

if(ENABLE_UI)

  # Qt is used for ecFlowUI only.
  # Algorithm: we test for Qt6 - if it's there, then use it; otherwise look for Qt5.
  #            if we don't find that, then we cannot build ecFlowUI.

  ecbuild_info( "Locating Qt" )
  ecbuild_info( "  note: searching for Qt6/Qt5 to support building ecFlowUI" )
  ecbuild_info( "  note: to use a self-built Qt installation, try setting CMAKE_PREFIX_PATH" )

  ecbuild_info( "Locating Qt6" )

  # Attempt to find Qt6 required components

  set(_qt6_required_components Widgets Gui Network Svg Core5Compat)
  set(_qt6_optional_component Charts)

  find_package(Qt6 COMPONENTS ${_qt6_required_components})

  if( Qt6_FOUND )

    # Attempt to find Qt6 optional components

    find_package(Qt6${_qt6_optional_component})

    ecbuild_info( " * Qt6_FOUND           : ${Qt6_FOUND}" )
    ecbuild_info( " * Qt6_VERSION         : ${Qt6_VERSION}" )
    ecbuild_info( " * Qt6 components" )
    foreach (_lib ${_qt6_required_components} ${_qt6_optional_component})
      if(Qt6${_lib}_FOUND)
        ecbuild_info( "   * ${_lib}, found ${Qt6${_lib}_LIBRARIES}" )
      endif()
    endforeach()

    if(Qt6Charts_FOUND)
      ecbuild_info( "Qt6Charts was found - the server log viewer will be built" )

      set(ECFLOW_LOGVIEW 1)
      add_definitions(-DECFLOW_LOGVIEW)

    else()
      ecbuild_info( "Qt6Charts was not found - the server log viewer will not be built" )
    endif()

    ecbuild_info( "Found Qt6 at ${Qt6_DIR}" )

  else()

    ecbuild_info( "Locating Qt5" )

    # Attempt to find Qt5 required components

    set(_qt5_required_components Widgets Gui Network Svg)
    set(_qt5_optional_component Charts)

    find_package(Qt5 COMPONENTS ${_qt5_required_components})

    if (Qt5_FOUND)

      # Attempt to find Qt5 optional components

      find_package(Qt5${_qt5_optional_component})

      ecbuild_info( " * Qt5_FOUND           : ${Qt5_FOUND}" )
      ecbuild_info( " * Qt5_VERSION         : ${Qt5_VERSION}" )
      ecbuild_info( " * Qt5 components" )
      foreach (_lib ${_qt5_required_components} ${_qt5_optional_components})
        if(Qt5${_lib}_FOUND)
          ecbuild_info( "   * ${_lib}, found ${Qt5${_lib}_LIBRARIES}" )
        endif()
      endforeach()

      if(Qt5Charts_FOUND)
        ecbuild_info( "  note: Qt5Charts was found - the server log viewer will be built" )

        set(ECFLOW_LOGVIEW 1)
        add_definitions(-DECFLOW_LOGVIEW)

      else()
        ecbuild_info( "  note: Qt5Charts was not found - the server log viewer will not be built" )
      endif()

      ecbuild_info( "Found Qt5 at ${Qt5_DIR}" )

    else()
      ecbuild_critical( "Qt5/6 not found - this is required for ecFlowUI; consider using -DENABLE_UI=OFF" )
    endif()
  endif()

  add_definitions(-DQT_NO_KEYWORDS) # We need to disable keywords because there is a problem in using Qt and boost together.

endif()

# =========================================================================================
# Doxygen
# =========================================================================================
ecbuild_info( "Locating Doxygen" )
find_package(Doxygen)
if (DOXYGEN_FOUND)
  ecbuild_info( "Found Doxygen at ${DOXYGEN_EXECUTABLE}" )

  # exclude some dirs not related to documentation
  set( DOXYGEN_EXCLUDE_PATTERNS */bin/* */bdir/* */Debug/*  */test/*  */Doc/* */doc/* */samples/* SCRATCH tools build_scripts cereal )

  set( DOXYGEN_SOURCE_BROWSER YES)
  set( DOXYGEN_EXTRACT_PRIVATE YES)
  set( DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Doc/doxygen")

  # this target will only be built if specifically asked to.
  # run "make doxygen" to create the doxygen documentation
  doxygen_add_docs(
    doxygen
    ${PROJECT_SOURCE_DIR}
    COMMENT "Generate documentation for ecFlow"
  )

  # Add an install target to install the docs, *IF* the use has run 'make doxygen'
  if (EXISTS ${DOXYGEN_OUTPUT_DIRECTORY})
    install(DIRECTORY ${DOXYGEN_OUTPUT_DIRECTORY} DESTINATION ${CMAKE_INSTALL_DOCDIR})
  endif()
else ()
  ecbuild_info( "Doxygen need to be installed to generate the doxygen documentation" )
endif()

# =========================================================================================
# Clang-format
# =========================================================================================
ecbuild_info( "Locating Clang-format" )
find_package(ClangFormat)
if (CLANGFORMAT_FOUND)
  ecbuild_info("Found Clang-Format at ${CLANGFORMAT_EXE} [${CLANGFORMAT_VERSION}]")
else()
  ecbuild_info("Clang-Format not found")
  ecbuild_info("    WARNING: No formatting targets will be defined!")
endif ()
