/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "UDPServerOptions.hpp"

#include "UDPServerEnvironment.hpp"

namespace ecf {

InvalidCLIOption::~InvalidCLIOption() = default;

namespace {

template <typename... Args>
std::string as_string(Args... args) {
    std::ostringstream os;
    ((os << args), ...);
    return os.str();
}

} // namespace

UDPServerOptions::UDPServerOptions(int argc, const char* argv[]) : general{create_options()}, variables{} {

    po::options_description all_options("All");
    all_options.add(general);

    try {
        // Collect command line options
        po::command_line_parser parser{argc, argv};
        parser.options(all_options)
            .style(po::command_line_style::default_style | po::command_line_style::allow_slash_for_short);
        auto cli_options = parser.run();
        po::store(cli_options, variables);

        // Collect environment options
        // note: by being processed later, environment options don't override cli options)
        UDPServerEnvironment environment;
        std::stringstream ss;
        ss << environment.as_configuration_file();
        auto env_options = po::parse_config_file(ss, all_options, true);
        po::store(env_options, variables);

        ensure_valid_options(variables);

        po::notify(variables);
    }
    catch (const po::error& error) {
        throw InvalidCLIOption(error.what());
    }
    catch (const InvalidCLIOption& error) {
        throw error;
    }
}

void UDPServerOptions::ensure_valid_options(const po::variables_map& variables) {
    if (variables.count(OPTION_HELP) > 0) {
        // Nothing to do, help will be displayed even if other options are provided
        return;
    }

    if (variables.count(OPTION_VERSION) > 0) {
        // Nothing to do, version will be displayed even if other options are provided
        return;
    }

    // Add all necessary options validation here!...
}

po::options_description UDPServerOptions::create_options() {
    po::options_description general{"General"};
    // clang-format off
    general.add_options()
        (as_string(OPTION_HELP, ",h").c_str(),
                        "Display options help")
        (as_string(OPTION_VERSION, ",v").c_str(),
                        "Display version information")
        (as_string(OPTION_VERBOSE).c_str(),
                        "Display debug information during execution")
        (as_string(OPTION_PORT, ",p").c_str(), po::value<size_t>()->default_value(8080),
                        "The port to listen for UDP packets")
        (as_string(OPTION_ECFLOW_HOST).c_str(), po::value<std::string>(),
                        "The ecFlow server port to forward requests")
        (as_string(OPTION_ECFLOW_PORT).c_str(), po::value<size_t>()->default_value(3141),
                        "The ecFlow server port to forward requests");
    // clang-format on

    return general;
}

} // namespace ecf
