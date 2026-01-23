/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_UDPServerOptions_HPP
#define ecflow_udp_UDPServerOptions_HPP

#include <optional>
#include <string>

#include <boost/program_options.hpp>

namespace ecf {

class InvalidCLIOption : public std::logic_error {
public:
    explicit InvalidCLIOption(const std::string& error) : logic_error(error) {}
    InvalidCLIOption(const InvalidCLIOption&) = default;
    ~InvalidCLIOption() override;
};

/**
 * Processes the CLI options used by ecFlow UDP server, including any environment variables that need to be considered.
 */
class UDPServerOptions {
public:
    UDPServerOptions(const std::vector<std::string>& argv);

    ~UDPServerOptions() = default;

    bool get_option(const std::string& option) const { return variables.count(option) > 0; }

    template <typename U, std::enable_if_t<!std::is_same_v<bool, U>, bool> = true>
    const U& get_option(const std::string& option) const {
        return variables[option].template as<U>();
    }

    template <typename U, std::enable_if_t<!std::is_same_v<bool, U>, bool> = true>
    std::optional<U> get_optional_option(const std::string& option) const {
        if (variables.count(option) > 0) {
            return variables[option].template as<U>();
        }
        return std::nullopt;
    }

    bool has_help() const { return get_option(OPTION_HELP); }
    bool has_version() const { return get_option(OPTION_VERSION); }
    bool has_verbose() const { return get_option(OPTION_VERBOSE); }

    bool has_http() const { return get_option(OPTION_ECFLOW_HTTP); }

    const boost::program_options::options_description& get_description() const { return general; }

    static boost::program_options::options_description create_options();

public:
    static inline const char* OPTION_HELP        = "help";
    static inline const char* OPTION_VERSION     = "version";
    static inline const char* OPTION_VERBOSE     = "verbose";
    static inline const char* OPTION_PORT        = "port";
    static inline const char* OPTION_ECFLOW_HOST = "ecflow_host";
    static inline const char* OPTION_ECFLOW_PORT = "ecflow_port";
    static inline const char* OPTION_ECFLOW_HTTP = "http";

private:
    static void ensure_valid_options(const boost::program_options::variables_map& variables);

    boost::program_options::options_description general;
    boost::program_options::variables_map variables;
};

} // namespace ecf

#endif /* ecflow_udp_UDPServerOptions_HPP */
