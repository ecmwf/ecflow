/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <string>

#include "ecflow/core/Version.hpp"
#include "ecflow/udp/RequestHandler.hpp"
#include "ecflow/udp/Trace.hpp"
#include "ecflow/udp/UDPServer.hpp"
#include "ecflow/udp/UDPServerOptions.hpp"

static std::string ecflow_udp_server_version() {
#ifdef DEBUG
    const std::string TAG = " (debug) ";
#else
    const std::string TAG = " ";
#endif

    std::ostringstream oss;
    oss << "Ecflow UDP" << TAG << "version(" << ecf::Version::full() << ")";
    return oss.str();
}

static void run_server(uint16_t port) {
    ecf::RequestHandler handler;
    ecf::UDPServer server{handler, port};
    server.run();
}

static int launch_server(const ecf::UDPServerOptions& options) {
    bool verbose = options.has_verbose();
    TRACE_VERBOSE(verbose)

    auto port = options.get_option<size_t>(ecf::UDPServerOptions::OPTION_PORT);
    TRACE_NFO("UDPServerMain", "starting server on port ", port)

    try {
        run_server(static_cast<uint16_t>(port));
    }
    catch (const std::exception& e) {
        TRACE_FATAL("UDPServerMain", e.what())
        return EXIT_FAILURE;
    }
    catch (...) {
        TRACE_FATAL("UDPServerMain", "unknown error detected")
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) try {

    ecf::UDPServerOptions options(argc, const_cast<const char**>(argv));

    if (options.has_help()) {
        std::cout << std::endl << "  ecFlow UDP" << std::endl << std::endl;
        std::cout << options.get_description() << std::endl;
        return EXIT_SUCCESS;
    }
    if (options.has_version()) {
        std::cout << ecflow_udp_server_version() << std::endl;
        return EXIT_SUCCESS;
    }

    {
        // Ensure the Client uses the ecFlow host+port
        auto ecflow_host = options.get_optional_option<std::string>(ecf::UDPServerOptions::OPTION_ECFLOW_HOST);
        auto ecflow_port = options.get_optional_option<size_t>(ecf::UDPServerOptions::OPTION_ECFLOW_PORT);
        if (ecflow_host) {
            setenv("ECF_HOST", ecflow_host.value().c_str(), 1);
        }
        if (ecflow_port) {
            setenv("ECF_PORT", std::to_string(ecflow_port.value()).c_str(), 1);
        }
        auto ecflow_http = options.has_http();
        if (ecflow_http) {
            setenv("ECF_HOST_PROTOCOL", "HTTPS", 1);
        }
        // Avoid that the Client automatically uses environment passwords
        unsetenv("ECF_PASSWD");
        unsetenv("ECF_CUSTOM_PASSWD");

        launch_server(options);
    }

    return EXIT_SUCCESS;
}
catch (ecf::InvalidCLIOption& e) {
    std::cout << "Error: Invalid CLI option detected: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (std::exception& e) {
    std::cout << "Error: Unexpected problem detected: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (...) {
    std::cout << "Error: Unknown problem detected. Cowardly giving up!..." << std::endl;
    return EXIT_FAILURE;
}
