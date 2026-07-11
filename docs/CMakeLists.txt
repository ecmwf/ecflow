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

ecbuild_info("Locating Sphinx")

find_package(Sphinx REQUIRED)

if(EXISTS ${SPHINX_EXECUTABLE})
  ecbuild_info("Sphinx found at ${SPHINX_EXECUTABLE}")
else()
  ecbuild_fatal_error("Sphinx executable not present at ${SPHINX_EXECUTABLE}")
endif()

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
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_CURRENT_BINARY_DIR}/client_api/api
      ${CMAKE_CURRENT_SOURCE_DIR}/client_api/api
  COMMAND
    ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/client_api/index.rst
      ${CMAKE_CURRENT_SOURCE_DIR}/client_api
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/client_api
  USES_TERMINAL
  DEPENDS ecflow_client
)
add_custom_target(ecflow_client_docs DEPENDS generate_ecflow_client_docs)


#
# (Re-)generate ecFlow Python documentation
#

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python_api/reference)
add_custom_command(
  OUTPUT generate_ecflow_python_docs
  COMMAND
    #
    # Clear the reference source directory to avoid stale files
    #
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_SOURCE_DIR}/python_api/reference/*
  COMMAND
    #
    # Clear the reference binary directory to avoid stale files
    #
    ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/python_api/reference/*
  COMMAND
    #
    # Generate the Python API documentation
    #
    #  * The API documentation is based on the docstrings in the Python module,
    #    extracted by introspection using the extract_api.py script
    #  * The API documentation .../python_api/reference/*.rst files are created in the binary directory
    #
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${ECFLOW_PYTHONPATH}"
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python_api/extract_api.py
  COMMAND
    #
    # Copy the categories list to the binary directory, to be used by python_api.py script
    #
    ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/python_api/categories.yaml
        ${CMAKE_CURRENT_BINARY_DIR}/python_api
  COMMAND
    #
    # Generate the Python API Reference index file, based on the reference files list
    #
    #  * The .../python_api/python_api.rst file is created in the binary directory
    #
    ${CMAKE_CURRENT_SOURCE_DIR}/python_api/python_api.py
  COMMAND
    #
    #  Copy the Python API Reference index file to the source directory
    #
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/python_api/python_api.rst ${CMAKE_CURRENT_SOURCE_DIR}/python_api
  COMMAND
    #
    #  Copy the API documentation .../python_api/reference/*.rst files to the source directory
    #
    ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/python_api/reference/* ${CMAKE_CURRENT_SOURCE_DIR}/python_api/reference/
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python_api
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
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${ECFLOW_PYTHONPATH}"
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
