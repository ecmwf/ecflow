#
# Copyright 2009-2023 ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

#
# Setup Sphinx
#

find_package(Sphinx REQUIRED)


#
# (Re-)generate ecFlow CLI documentation
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/client_api/api)
add_custom_command(
  OUTPUT generate_ecflow_client_docs
  COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/client_api/api/*
  COMMAND
    ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin:$ENV{PATH}"
        ${CMAKE_CURRENT_SOURCE_DIR}/client_api/build.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/client_api/api
  USES_TERMINAL
  DEPENDS ecflow_client
)
add_custom_target(ecflow_client_docs DEPENDS generate_ecflow_client_docs)


#
# (Re-)generate ecFlow Python documentation
#

message(STATUS "***> ${SPHINX_EXECUTABLE}")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/rst)
add_custom_command(
  OUTPUT generate_ecflow_python_docs
  COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/python_api/*
  COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/rst/*
  COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/_build
  COMMAND
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_BINARY_DIR}/Pyext/python3"
        ${SPHINX_EXECUTABLE} -M html
          ${CMAKE_CURRENT_SOURCE_DIR}/build_python_api
          ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/_build
  COMMAND
    ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/build_python_api/glossary.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/build_python_api/categories.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/build_python_api
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/build_python_api/fix_rst.py
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/build_python_api/build_toc.py
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/python_api.rst ${CMAKE_CURRENT_SOURCE_DIR}/python_api
  COMMAND
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/build_python_api/rst/* ${CMAKE_CURRENT_SOURCE_DIR}/python_api
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build_python_api
  USES_TERMINAL
  DEPENDS ecflow3
)
add_custom_target(ecflow_python_docs DEPENDS generate_ecflow_python_docs)

#
# Build ecFlow documentation
#

add_custom_command(
  OUTPUT generate_ecflow_docs
  COMMAND
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_BINARY_DIR}/Pyext/python3"
      ${SPHINX_EXECUTABLE} -M html
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}/_build
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  USES_TERMINAL
  DEPENDS
    ecflow_client_docs
    ecflow_python_docs
)
add_custom_target(ecflow_docs DEPENDS generate_ecflow_docs)
