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
# ecflow_add_test
#
# Declare a test target, as a thin wrapper around ecbuild_add_test().
#
#   ecflow_add_test(
#     TARGET <target>
#     <ecbuild_add_test arguments>...
#     [CONDITION <condition>...]
#     [FORMAT]
#   )
#
# All arguments are forwarded verbatim to ecbuild_add_test(); refer to the
# documentation of that function for the full set of supported options.
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

  if (ARGS_FORMAT AND DEFINED ARGS_TARGET)
    if (TARGET ${ARGS_TARGET})
      set(format_args ${ARGS_TARGET})
      if (DEFINED ARGS_CONDITION)
        list(APPEND format_args CONDITION ${ARGS_CONDITION})
      endif()
      target_clangformat(${format_args})
    endif()
  endif()
endfunction()
