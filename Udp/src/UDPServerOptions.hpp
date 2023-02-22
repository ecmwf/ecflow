/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_UDP_UDPSERVEROPTIONS_HPP
#define ECFLOW_UDP_UDPSERVEROPTIONS_HPP

#include <optional>
#include <string>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace ecf {

class InvalidCLIOption : public std::logic_error {
public:
    explicit InvalidCLIOption(const std::string& error) : logic_error(error) {}
    InvalidCLIOption(const InvalidCLIOption&) = default;
    virtual ~InvalidCLIOption();
};

/**
 * Processes the CLI options used by ecFlow UDP server, including any environment variables that need to be considered.
 */
class UDPServerOptions {
public:
    UDPServerOptions(int argc, const char* argv[]);

    ~UDPServerOptions() = default;

    bool get_option(const std::string& option) const { return variables.count(option) > 0; }

    template <typename U, typename = std::enable_if_t<!std::is_same_v<bool, U>, void>>
    const U& get_option(const std::string& option) const {
        return variables[option].template as<U>();
    }

    template <typename U, typename = std::enable_if_t<!std::is_same_v<bool, U>, void>>
    std::optional<U> get_optional_option(const std::string& option) const {
        if (variables.count(option) > 0) {
            return variables[option].template as<U>();
        }
        return {};
    }

    bool has_help() const { return get_option(OPTION_HELP); }
    bool has_version() const { return get_option(OPTION_VERSION); }
    bool has_verbose() const { return get_option(OPTION_VERBOSE); }

    const po::options_description& get_description() const { return general; }

    static po::options_description create_options();

public:
    static inline const char* OPTION_HELP        = "help";
    static inline const char* OPTION_VERSION     = "version";
    static inline const char* OPTION_VERBOSE     = "verbose";
    static inline const char* OPTION_PORT        = "port";
    static inline const char* OPTION_ECFLOW_HOST = "ecflow_host";
    static inline const char* OPTION_ECFLOW_PORT = "ecflow_port";

private:
    static void ensure_valid_options(const po::variables_map& variables);

    po::options_description general;
    po::variables_map variables;
};

} // namespace ecf

#endif
