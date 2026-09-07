#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

#
#   Important!
#
# Notice that ecflow Python module is linked with `Python3::Module` CMake imported target,
# to enable a loose linkage with Python development libraries -- this is necessary in some
# platforms, such as conda-forge+macOS.
#
# In practice, this implies that `-undefined` and `-dynamic_lookup` linkage flags are used
# and the Python development libraries are not linked directly into the ecflow Python module.
#

ecbuild_add_library(
  TARGET
    ecflow3
  NOINSTALL
  TYPE MODULE
  SOURCES
    ${srcs}
  PRIVATE_INCLUDES
    ../src
  PUBLIC_LIBS
    ecflow_all
    libsimulator
    Python3::Module
    pybind11::headers
    $<$<BOOL:${OPENSSL_FOUND}>:OpenSSL::SSL>
  CXXFLAGS
    $<$<CXX_COMPILER_ID:Clang>:-Wno-macro-redefined>
)

target_clangformat(ecflow3)

set(ECFLOW_PYTHONPATH ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "Path to find ecflow python module for testing" FORCE)

#
# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#         
# by default cmake add prefix 'lib', we don't want this hence disable
#
# To avoid duplicate target names we chose to name the targets: ecflow2/ecflow3.
# however test and user code  depend on name 'ecflow', hence we rename the output to 'cflow'
set_target_properties(ecflow3
  PROPERTIES
    OUTPUT_NAME "ecflow"
    PREFIX ""
    INSTALL_RPATH ""
)

# =====================================================================
# tests

# The python tests are executed via 'python -m pytest' (see below), so pytest
# must be available for the selected interpreter. Detect it at configure time,
# rather than having every py3_u_* / py3_s_* test fail at run time with
# 'No module named pytest'.
#
# Presence only. The `pytest >= 8.0` in requirements-test.txt is what a fresh
# install should get, not a floor the tests need: they run fine on the pytest 7
# that distros ship, and enforcing it here would reject working environments.
execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import pytest"
  RESULT_VARIABLE _ECFLOW_HAVE_PYTEST
  OUTPUT_QUIET ERROR_QUIET)

# One quoted string, not a list: ';' is CMake's list separator, so a message
# assembled from several arguments loses any literal semicolon in it -- and
# ecbuild_critical() replaces the separators with spaces on top of that. Hence
# the '.' below where a ';' would read better.
if( NOT _ECFLOW_HAVE_PYTEST EQUAL 0 )
  set( _ECFLOW_PYTEST_HINT
    "pytest was not found for ${Python3_EXECUTABLE}. The python tests (py3_u_* and py3_s_*) will fail to run. Install it with:\n    ${Python3_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/../test/requirements-test.txt" )
  # ENABLE_ALL_TESTS is what CI and the .ci/hpc recipes gate the full suite on, so
  # there a missing pytest is a broken environment, not a note: fail now rather
  # than at ctest time, where it reads as 79 unrelated test failures.
  # HAVE_TESTS because ecbuild_add_test() registers nothing without it, leaving
  # nothing that could fail.
  if( HAVE_TESTS AND ENABLE_ALL_TESTS )
    ecbuild_critical( "${_ECFLOW_PYTEST_HINT}\nAlternatively, consider using -DENABLE_ALL_TESTS=OFF" )
  else()
    message( WARNING "${_ECFLOW_PYTEST_HINT}" )
  endif()
endif()

#
# When using some HPC toolchains, the dynamic linker for ecflow resolves and loads the platform default libstdc++ first,
# instead of the one that ecflow.so was built against. This leads to symbol version mismatches, and the python tests
# fail with errors like:
#
#   /lib64/libstdc++.so.6: error: version lookup error: version `GLIBCXX_3.4.32' not found
#     (required by /path/to/ecflow/build/libs/pyext/python3/ecflow.so) (fatal)
#
# ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH allows a runtime library directory to be prepended to LD_LIBRARY_PATH, so that
# the correct libstdc++.so.6 is used by the python tests.
#
set(ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH "" CACHE STRING
  "Colon-separated directories prepended to LD_LIBRARY_PATH when running the python tests")

# A handful of tests assert behaviour that a given Boost introduced -- the
# gregorian 1400-9999 year range, from 1.67.0 -- and the range our platforms span
# straddles it (rocky 8 ships 1.66). Hand the version to the tests so they can
# skip on the thing that actually decides, rather than on a runner or image name.
set( _ECFLOW_PYEXT_ENVIRONMENT
  "PATH=${Python3_EXECUTABLE_DIR}:$ENV{PATH}"
  "PYTHONPATH=${ECFLOW_PYTHONPATH}"
  "ECFLOW_TEST_BOOST_VERSION=${Boost_VERSION_STRING}"
)
if( ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH )
  if( DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "" )
    list( APPEND _ECFLOW_PYEXT_ENVIRONMENT "LD_LIBRARY_PATH=${ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH}" )
  else()
    list( APPEND _ECFLOW_PYEXT_ENVIRONMENT "LD_LIBRARY_PATH=${ECFLOW_PYEXT_TEST_LD_LIBRARY_PATH}" )
  endif()
endif()

foreach( test ${u_tests} )
  ecbuild_add_test(
    TARGET
      py3_${test}
    LABELS
      unit python nightly fast
    TYPE PYTHON
    ARGS
      -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/../test/py_${test}.py -v
    ENVIRONMENT
      ${_ECFLOW_PYEXT_ENVIRONMENT}
    TEST_DEPENDS
      u_base
  )
endforeach()


if ( ENABLE_ALL_TESTS AND ENABLE_SERVER)

  #
  # Python integration tests run the ecflow server, and thus use ECF_SSL_DIR to find the generated SSL certificates.
  #
  list( APPEND _ECFLOW_PYEXT_ENVIRONMENT "ECF_SSL_DIR=${CMAKE_CURRENT_BINARY_DIR}/" )

  foreach( test ${s_tests} )
    ecbuild_add_test(
      TARGET
        py3_${test}
      LABELS
        integration python nightly slow
      TYPE PYTHON
      ARGS
        -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/../test/py_${test}.py -v
      ENVIRONMENT
        ${_ECFLOW_PYEXT_ENVIRONMENT}
      TEST_DEPENDS
        u_base
    )
  endforeach()
  
  set_property(TEST py3_s_TestClientApi      APPEND PROPERTY DEPENDS s_test)
  set_property(TEST py3_s_TestPythonChildApi APPEND PROPERTY DEPENDS py3_s_TestClientApi)
endif()


# ==========================================================================
# install
#    -DCMAKE_PYTHON_INSTALL_TYPE = [ local | setup | not defined ]
#
#    local | not defined : this will install to:
#                          $INSTALL_PREFIX/$release.$major.$minor/lib/python2.7/site-packages/ecflow/
#    setup               : experimental only,python way of installing
#
#    -DCMAKE_PYTHON_INSTALL_PREFIX should *only* used when using python setup.py (CMAKE_PYTHON_INSTALL_TYPE=setup)
#    *AND* for testing python install to local directory
#
# Note:  To install only the python module
#       cd  buildir
#       cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCOMPONENT=python -P cmake_install.cmake -- make install
# ==========================================================================

if( CMAKE_PYTHON_INSTALL_TYPE MATCHES  "local"  OR NOT DEFINED CMAKE_PYTHON_INSTALL_TYPE )

  if( NOT INSTALL_PYTHON3_DIR )
    set(PYTHON_SITE "lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages" )
  else()
    # Resolve ${VAR} in the value provided
    string(CONFIGURE "${INSTALL_PYTHON3_DIR}" PYTHON_SITE)
  endif()
  set(PYTHON_DEST "${PYTHON_SITE}/ecflow" )

  install(
    TARGETS
      ecflow3
    DESTINATION
      ${PYTHON_DEST}
    RENAME
      ecflow.so
    COMPONENT
      python
  )
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/../ecflow/__init__.py"
      ../samples/api/ecf.py
      ../samples/api/sms2ecf.py
    DESTINATION
      ${PYTHON_DEST}
    COMPONENT
      python
  )

else()

  message( STATUS "python found,  CMAKE_PYTHON_INSTALL_TYPE=${CMAKE_PYTHON_INSTALL_TYPE}")

  # -------------------------------------------------------------------------------------
  # Install using setup.py
  # See: http://bloerg.net/2012/11/10/cmake-and-distutils.html
  # -------------------------------------------------------------------------------------
  message(STATUS "python install using *setup.py* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
  message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
  message(STATUS "CMAKE_PYTHON_INSTALL_PREFIX : ${CMAKE_PYTHON_INSTALL_PREFIX}" )

  set(OUTPUT      "${CMAKE_CURRENT_SOURCE_DIR}/timestamp")

  # Notice:
  #  (1) SETUP_PY and INIT_PY files are generated from .in files in the binary directory
  #  (2) these files are configured by the .../pyext/CMakeLists.txt

  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${PYTHON} ${SETUP_PY} build
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    DEPENDS ${INIT_PY}
  )
  add_custom_target(target ALL DEPENDS ${OUTPUT})

  install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} build_ext)")

  if( DEFINED CMAKE_PYTHON_INSTALL_PREFIX )
    message(STATUS "custom/*test* python install prefix defined CMAKE_PYTHON_INSTALL_PREFIX=${CMAKE_PYTHON_INSTALL_PREFIX}")
    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install -f --prefix=${CMAKE_PYTHON_INSTALL_PREFIX})")
  else()
    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
  endif()
endif()
