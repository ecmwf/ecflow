/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/udp/UDPServerEnvironment.hpp"

#include "ecflow/core/Environment.hpp"

namespace ecf {

namespace /* __anonymous__ */ {

// all variables to be collected
std::array<const char*, 4> variables = {UDPServerEnvironment::ECF_UDP_VERBOSE,
                                        UDPServerEnvironment::ECF_UDP_PORT,
                                        UDPServerEnvironment::ECF_HOST,
                                        UDPServerEnvironment::ECF_PORT};

// the options related to each of the variables
const std::unordered_map<std::string, std::string> options_map = {{UDPServerEnvironment::ECF_UDP_VERBOSE, "verbose"},
                                                                  {UDPServerEnvironment::ECF_UDP_PORT, "port"},
                                                                  {UDPServerEnvironment::ECF_HOST, "ecflow_host"},
                                                                  {UDPServerEnvironment::ECF_PORT, "ecflow_port"}};

} // namespace

UDPServerEnvironment::UDPServerEnvironment()
    : environment_{} {
    for (auto variable : variables) {
        ecf::environment::get(variable, environment_[variable]);
    }
}

std::string UDPServerEnvironment::as_configuration_file() const {
    std::ostringstream ss;
    for (const auto& entry : environment_) {
        if (auto found = options_map.find(entry.first); found != std::end(options_map) && !std::empty(entry.second)) {
            ss << found->second << "=" << entry.second << std::endl;
        }
    }
    return ss.str();
}

} // namespace ecf
