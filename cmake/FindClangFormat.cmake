#
# Copyright 2023- ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

#
# ClangFormat.cmake allows to easily setup source code formatting
#
# Use this module by including it with:
#
#   find_package(ClangFormat)
#
# And then configure each project target with:
#
#   target_clangformat(<TARGET> [CONDITION <CONDITION>])
#
# This will automatically create the following targets:
#
#   clangformat               - executes of all format targets
#   clangformat_<target-name> - formats the sources of a specific executable/library target
#                               (n.b. only files declared as target sources are formatted;
#                                     #include files are not considered unless explicitly listed as target sources)
#
# If CONDITION is supplied and evaluates to FALSE, these targets will not be created
#

function(target_clangformat TARGET)

  set(options "")
  set(single_value_args CONDITION)
  set(multi_value_args "")
  cmake_parse_arguments( ARGS "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN} )

  # Skip setup if condition supplied and FALSE
  if (NOT ${ARGS_CONDITION})
    return()
  endif()

  # Skip setup if clang-format is not available...
  if (NOT CLANGFORMAT_EXE)
    return()
  endif ()

  # Collect list of target sources sources
  get_target_property(target_sources ${TARGET} SOURCES)
  foreach (clangformat_source ${target_sources})
    get_filename_component(clangformat_source ${clangformat_source} ABSOLUTE)
    list(APPEND clangformat_sources ${clangformat_source})
  endforeach ()

  # Define name of specific format target
  get_target_property(target_name ${TARGET} NAME)
  set(format_target clangformat_${target_name})

  # Add custom specific format target
  add_custom_target(${format_target}
    COMMAND
    ${CLANGFORMAT_EXE}
    -style=file
    -i
    ${clangformat_sources}
    COMMENT
    "Formatting '${TARGET}' with ${CLANGFORMAT_EXE} ..."
    )

  # Wire up dependencies to main format target
  if (TARGET clangformat)
    add_dependencies(clangformat ${format_target})
  else ()
    add_custom_target(clangformat DEPENDS ${format_target})
  endif ()
endfunction()

function(clangformat_get_version VAR)
  execute_process(COMMAND ${CLANGFORMAT_EXE} -version OUTPUT_VARIABLE VERSION_OUTPUT)
  separate_arguments(VERSION_OUTPUT_LIST UNIX_COMMAND "${VERSION_OUTPUT}")
  list(FIND VERSION_OUTPUT_LIST "version" VERSION_INDEX)
  if (VERSION_INDEX GREATER 0)
    math(EXPR VERSION_INDEX "${VERSION_INDEX} + 1")
    list(GET VERSION_OUTPUT_LIST ${VERSION_INDEX} VERSION)
    set(${VAR} ${VERSION} PARENT_SCOPE)
  else ()
    set(${VAR} "<(n/a)>" PARENT_SCOPE)
  endif ()
endfunction()


# __MAIN__ (the following is evaluated immediately when the module is included)

find_program(CLANGFORMAT_EXE
  NAMES
  clang-format
  clang-format-6
  clang-format-7
  clang-format-8
  clang-format-9
  clang-format-11
  clang-format-12
  clang-format-13
  clang-format-14
  clang-format-15
  clang-format-16
  PATHS
  /usr/bin
  /usr/local/opt/llvm/bin
  )

if (NOT CLANGFORMAT_EXE)
  message(STATUS "Clang-Format not found")
  message(STATUS "    WARNING: No formatting targets will be defined!")
else ()
  clangformat_get_version(CLANGFORMAT_VERSION)
  message(STATUS "Clang-Format found at ${CLANGFORMAT_EXE} [${CLANGFORMAT_VERSION}]")
  set(CLANGFORMAT_FOUND ON)
endif ()
