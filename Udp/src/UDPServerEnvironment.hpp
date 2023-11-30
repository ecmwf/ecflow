/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_UDPServerEnvironment_HPP
#define ecflow_udp_UDPServerEnvironment_HPP

#include <optional>
#include <string>
#include <vector>

#include <unordered_map>

#include "ecflow/core/Converter.hpp"

namespace ecf {

/**
 * Allows to collect information about environment variables related to ecFlow UDP,
 * and eventually customizes the options used to launch the server.
 */
class UDPServerEnvironment {
private:
    using storage_t = std::unordered_map<std::string, std::string>;

public:
    UDPServerEnvironment();

    bool get_variable(const std::string& key) const {
        auto found = environment_.find(key);
        return found != std::end(environment_);
    }

    template <typename U, std::enable_if_t<!std::is_same_v<bool, U>, bool> = true>
    std::optional<U> get_variable(const std::string& key) const {
        auto found = environment_.find(key);
        if (found != std::end(environment_)) {
            return ecf::convert_to<U>(found->second);
        }
        return {};
    }

    std::string as_configuration_file() const;

public:
    static constexpr const char* ECF_UDP_VERBOSE = "ECF_UDP_VERBOSE";
    static constexpr const char* ECF_UDP_PORT    = "ECF_UDP_PORT";
    static constexpr const char* ECF_HOST        = "ECF_HOST";
    static constexpr const char* ECF_PORT        = "ECF_PORT";

private:
    storage_t environment_;
};

} // namespace ecf

#endif /* ecflow_udp_UDPServerEnvironment_HPP */
