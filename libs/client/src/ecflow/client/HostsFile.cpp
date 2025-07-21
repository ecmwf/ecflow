/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/client/HostsFile.hpp"

#include <fstream>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"

namespace ecf {

HostsFile HostsFile::parse(const fs::path& path, const int default_port) {
    // Open the file
    std::ifstream ifs(path.c_str(), std::ios_base::in);
    if (!ifs) {
        THROW_EXCEPTION(HostsFileFailure, "Could not load the hosts file content: " << path);
    }

    return HostsFile::parse(ifs, default_port);
}

HostsFile HostsFile::parse(std::istream& is, const int default_port) {

    // Load the file content into lines
    auto lines = File::splitStreamIntoLines(is, true);

    // Process each line and extract hosts and ports
    std::vector<std::pair<std::string, std::string>> hosts;

    for (const auto& line : lines) {

        std::vector<std::string> tokens;
        ecf::Str::split(line, tokens);
        if (tokens.empty() || tokens[0].empty() || tokens[0][0] == '#') {
            continue; // skip empty lines and comments
        }

        std::string host;
        std::string port = std::to_string(default_port);

        size_t separator = tokens[0].find(':');
        if (separator != std::string::npos) {
            host = tokens[0].substr(0, separator);
            port = tokens[0].substr(separator + 1);

            // Validate the port number
            if (port.empty()) {
                // The port can be empty, in which case we use the default port
                port = std::to_string(default_port);
            }
            else if (!ecf::Str::is_int(port)) {
                // If the port must be an integer, otherwise throw an exception
                THROW_EXCEPTION(HostsFileFailure, "Non-integer port number in hosts file: " << port);
            }
            else if (int number = ecf::convert_to<int>(port); number < 0 || number > 65535) {
                // If the port is not in the correct range, throw an exception
                THROW_EXCEPTION(HostsFileFailure, "Out-of-range port number in hosts file: " << port);
            }
        }
        else {
            host = tokens[0];
        }

        if (!host.empty()) {
            hosts.emplace_back(host, port);
        }
    }

    return HostsFile(std::move(hosts));
}

} // namespace ecf
