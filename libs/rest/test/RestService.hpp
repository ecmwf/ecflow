// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include "PermissionsServer.hpp"
#include "ecflow/core/HttpLibrary.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace ecf::test {

///
/// @brief Runs the REST service, without SSL, against a server run by PermissionsServer.
///
/// The service runs as a process of its own, in a directory of its own, and refreshes its copy of the tree as the user
/// running the test, without a password: it ignores the password files of clients, and the server must therefore be
/// open to the login user (see PermissionsServer). The port of the service is reserved in the given range, and
/// released when the service is stopped. The service is killed when the object is destroyed.
///
class RestService {
public:
    ///
    /// @brief Starts the service, and waits for it to answer.
    ///
    /// @param[in] server The server the service connects to; it must outlive the service
    /// @param[in] min The lowest port of the range reserved for the service
    /// @param[in] max The highest port of the range reserved for the service
    /// @throws std::runtime_error when no port of the range can be reserved, or the service does not answer within
    ///         30 s; the service is then killed, and its output reported.
    ///
    RestService(const PermissionsServer& server, int min, int max)
        : directory_{scaffold::MakeDirectory{}.create()},
          port_{reserve(min, max)} {
        const auto tokens = directory_.path() / "tokens.json";
        std::ofstream(tokens) << "[]\n";

        // The service takes its identity, and the server, from the environment when it starts
        scaffold::WithoutTestEnvironmentVariable user("ECF_USER");
        scaffold::WithoutTestEnvironmentVariable protocol("ECF_HOST_PROTOCOL");
        scaffold::WithTestEnvironmentVariable host("ECF_HOST", "localhost");
        scaffold::WithTestEnvironmentVariable server_port("ECF_PORT", server.port());

        const std::vector<std::string> args{"--no_ssl",
                                            "--port",
                                            port(),
                                            "--ecflow_host",
                                            "localhost",
                                            "--ecflow_port",
                                            server.port(),
                                            "--polling_interval",
                                            "1",
                                            "--tokens_file",
                                            tokens.string()};
        const auto executable = fs::path(File::find_ecf_server_path()).parent_path() / "ecflow_http";
        process_              = std::make_unique<scaffold::Process>(executable, args, directory_.path());

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!get("/v1/server/ping")) {
            if (std::chrono::steady_clock::now() > deadline || !process_->is_running()) {
                const auto message = "RestService: the service did not answer on port " + port();
                stop();
                throw std::runtime_error(message + ", output:\n" + process_->read_stdout() + process_->read_stderr());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    RestService(const RestService&)            = delete;
    RestService& operator=(const RestService&) = delete;

    ~RestService() { stop(); }

    ///
    /// @brief Sends a GET request to the service.
    ///
    /// @param[in] path The path of the request, e.g. `/v1/suites`
    /// @return The response, or an empty result when the service does not answer.
    ///
    [[nodiscard]] httplib::Result get(const std::string& path) const {
        httplib::Client client("localhost", std::stoi(port()));
        client.set_connection_timeout(std::chrono::seconds(5));
        client.set_read_timeout(std::chrono::seconds(10));
        return client.Get(path);
    }

    ///
    /// @brief Kills the service, if it runs, and releases its port.
    ///
    void stop() {
        if (process_) {
            process_->terminate();
        }
        port_.reset();
    }

    ///
    /// @brief Returns the port of the service.
    ///
    /// @return The port, reserved until the service is stopped; empty once stopped.
    ///
    [[nodiscard]] std::string port() const { return port_ ? std::to_string(port_->value()) : std::string{}; }

private:
    static std::unique_ptr<scaffold::Port> reserve(int min, int max) {
        for (int port = min; port <= max; ++port) {
            try {
                return scaffold::MakePort{}
                    .with(scaffold::SpecificPortValue{static_cast<scaffold::Port::port_t>(port)})
                    .create_owned();
            }
            catch (const scaffold::MakePort::UnableToLockPort&) {
                // locked by another test, or bound: try the next port
            }
        }
        throw std::runtime_error("RestService: no free port in [" + std::to_string(min) + ", " + std::to_string(max) +
                                 "]");
    }

    scaffold::Directory directory_;
    std::unique_ptr<scaffold::Port> port_;
    std::unique_ptr<scaffold::Process> process_;
};

} // namespace ecf::test
