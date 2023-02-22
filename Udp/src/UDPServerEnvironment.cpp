/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "UDPServerEnvironment.hpp"

#include <cstdio>

namespace ecf {

namespace /* __anonymous__ */ {

// all variables to be collected
const char* const variables[] = {UDPServerEnvironment::ECF_UDP_VERBOSE,
                                 UDPServerEnvironment::ECF_UDP_PORT,
                                 UDPServerEnvironment::ECF_HOST,
                                 UDPServerEnvironment::ECF_PORT};

// the options related to each of the variables
const std::unordered_map<std::string, std::string> options_map = {{UDPServerEnvironment::ECF_UDP_VERBOSE, "verbose"},
                                                                  {UDPServerEnvironment::ECF_UDP_PORT, "port"},
                                                                  {UDPServerEnvironment::ECF_HOST, "ecflow_host"},
                                                                  {UDPServerEnvironment::ECF_PORT, "ecflow_port"}};

} // namespace

UDPServerEnvironment::UDPServerEnvironment() : environment_{} {
    for (auto variable : variables) {
        if (const char* value = ::getenv(variable); value) {
            environment_[variable] = value;
        }
    }
}

std::string UDPServerEnvironment::as_configuration_file() const {
    std::ostringstream os;
    for (const auto& entry : environment_) {
        if (auto found = options_map.find(entry.first); found != std::end(options_map)) {
            os << found->second << "=" << entry.second << std::endl;
        }
    }
    return os.str();
}

} // namespace ecf
