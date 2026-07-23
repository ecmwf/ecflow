# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

#
# Generates a C++ header embedding docs/client_api/help.json as a raw string
# literal, so ecflow_client can carry its own CLI help data without reading an
# external file at runtime.
#
# Invoked as a build-time custom command (not configure_file), so the header
# regenerates whenever the manifest changes, without requiring a full CMake
# reconfigure.
#
# Expected variables (passed via -D on the command line):
#   MANIFEST   path to docs/client_api/help.json
#   OUTPUT     path to the generated header to write
#

if(NOT DEFINED MANIFEST)
  message(FATAL_ERROR "GenerateClientHelp.cmake: MANIFEST is not set")
endif()
if(NOT DEFINED OUTPUT)
  message(FATAL_ERROR "GenerateClientHelp.cmake: OUTPUT is not set")
endif()

file(READ "${MANIFEST}" manifest_content)

if(manifest_content MATCHES "\\)ecflow_help\"")
  message(FATAL_ERROR
    "GenerateClientHelp.cmake: ${MANIFEST} contains the raw-string delimiter "
    ")ecflow_help\", which would break the generated header. Pick a different "
    "delimiter here and in generated_client_help.hpp's consumers.")
endif()

file(WRITE "${OUTPUT}" "/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

/*
 * Generated from docs/client_api/help.json -- see cmake/GenerateClientHelp.cmake.
 *
 * DO NOT EDIT DIRECTLY.
 */

#ifndef ecflow_base_generated_client_help_HPP
#define ecflow_base_generated_client_help_HPP

#include <string_view>

inline constexpr std::string_view client_help_json = R\"ecflow_help(
${manifest_content})ecflow_help\";

#endif /* ecflow_base_generated_client_help_HPP */
")
