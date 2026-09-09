/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_udp_UDPServerEnvironment_HPP
#define ecflow_udp_UDPServerEnvironment_HPP

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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
