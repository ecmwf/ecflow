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
# BuildUtilities.cmake provides thin wrappers around the ecbuild target
# declaration functions, adding the FORMAT option so that a target and the
# formatting of its sources are declared together.
#

#
# _ecflow_format_target(<target> [CONDITION <condition>...])
#
# Register the sources of <target> with target_clangformat(), provided the
# target has actually been created. The remaining arguments are forwarded
# verbatim to target_clangformat().
#
# This is an implementation detail of the wrappers below and is not intended
# to be called directly.
#
function(_ecflow_format_target target)
  if (NOT target)
    return()
  endif()
  if (NOT TARGET ${target})
    return()
  endif()

  target_clangformat(${target} ${ARGN})
endfunction()

#
# ecflow_add_library
#
# Declare a library target, as a thin wrapper around ecbuild_add_library().
#
#   ecflow_add_library(
#     TARGET <target>
#     [CONDITION <condition>...]
#     [FORMAT]
#     <ecbuild_add_library arguments>...
#   )
#
# All other arguments are forwarded verbatim to ecbuild_add_library(); refer
# to the documentation of that function for the full set of supported options.
#
# TARGET : required
#   The name of the library target to create.
#
# CONDITION : optional
#   A list of tokens forming a boolean expression, evaluated as a CMake if()
#   condition. When the expression evaluates to FALSE, neither the library
#   target nor the associated formatting target is created. The expression is
#   forwarded both to ecbuild_add_library() and, when FORMAT is given, to
#   target_clangformat(), so that both remain governed by the same condition.
#
# FORMAT : optional
#   Register the sources of the library target with target_clangformat(),
#   thereby creating the clangformat_<target> target and attaching it to the
#   aggregate clangformat target. The formatting target is only created when
#   the library target itself has been created.
#
function(ecflow_add_library)
  set(options FORMAT)
  set(single_value_args TARGET)
  set(multi_value_args CONDITION)
  cmake_parse_arguments(ARGS "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN})

  set(library_args ${ARGS_UNPARSED_ARGUMENTS})
  if (DEFINED ARGS_TARGET)
    list(PREPEND library_args TARGET ${ARGS_TARGET})
  endif()
  if (DEFINED ARGS_CONDITION)
    list(APPEND library_args CONDITION ${ARGS_CONDITION})
  endif()

  ecbuild_add_library(${library_args})

  if (ARGS_FORMAT)
    set(format_args "")
    if (DEFINED ARGS_CONDITION)
      set(format_args CONDITION ${ARGS_CONDITION})
    endif()
    _ecflow_format_target("${ARGS_TARGET}" ${format_args})
  endif()
endfunction()

#
# ecflow_add_test
#
# Declare a test target, as a thin wrapper around ecbuild_add_test().
#
#   ecflow_add_test(
#     TARGET <target>
#     [CONDITION <condition>...]
#     [FORMAT]
#     <ecbuild_add_test arguments>...
#   )
#
# All other arguments are forwarded verbatim to ecbuild_add_test(); refer to
# the documentation of that function for the full set of supported options.
#
# TARGET : required
#   The name of the test target to create.
#
# CONDITION : optional
#   A list of tokens forming a boolean expression, evaluated as a CMake if()
#   condition. When the expression evaluates to FALSE, neither the test target
#   nor the associated formatting target is created. The expression is
#   forwarded both to ecbuild_add_test() and, when FORMAT is given, to
#   target_clangformat(), so that both remain governed by the same condition.
#
# FORMAT : optional
#   Register the sources of the test target with target_clangformat(), thereby
#   creating the clangformat_<target> target and attaching it to the aggregate
#   clangformat target. The formatting target is only created when the test
#   target itself has been created.
#
function(ecflow_add_test)
  set(options FORMAT)
  set(single_value_args TARGET)
  set(multi_value_args CONDITION)
  cmake_parse_arguments(ARGS "${options}" "${single_value_args}" "${multi_value_args}" ${ARGN})

  set(test_args ${ARGS_UNPARSED_ARGUMENTS})
  if (DEFINED ARGS_TARGET)
    list(PREPEND test_args TARGET ${ARGS_TARGET})
  endif()
  if (DEFINED ARGS_CONDITION)
    list(APPEND test_args CONDITION ${ARGS_CONDITION})
  endif()

  ecbuild_add_test(${test_args})

  if (ARGS_FORMAT)
    set(format_args "")
    if (DEFINED ARGS_CONDITION)
      set(format_args CONDITION ${ARGS_CONDITION})
    endif()
    _ecflow_format_target("${ARGS_TARGET}" ${format_args})
  endif()
endfunction()
