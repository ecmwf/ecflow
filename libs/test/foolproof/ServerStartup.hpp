// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iterator>
#include <list>
#include <stdexcept>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

///
/// Helpers shared by the s_foolproof.server_startup suites, which launch ecflow_server directly to check its start-up.
///
namespace ecf::test::foolproof {

///
/// @brief Clears, for the lifetime of the object, every environment variable read by ecflow_server or its clients.
///
/// Settings applied after construction (for example with WithTestEnvironmentVariable) are not affected.
///
class ClearedServerEnvironment {
public:
    ClearedServerEnvironment() {
        for (const auto* name : {"ECF_HOME",
                                 "ECF_PORT",
                                 "ECF_LOG",
                                 "ECF_CHECK",
                                 "ECF_CHECKOLD",
                                 "ECF_PASSWD",
                                 "ECF_CUSTOM_PASSWD",
                                 "ECF_LISTS",
                                 "ECF_CHECKINTERVAL",
                                 "ECF_PRUNE_NODE_LOG",
                                 "ECF_TASK_THRESHOLD",
                                 "ECF_SSL",
                                 "ECF_DEBUG_SERVER",
                                 "ECF_USER",
                                 "ECF_HOST",
                                 "ECF_PASS",
                                 "ECF_PASSWORD"}) {
            cleared_.emplace_back(name);
        }
    }

private:
    std::list<ecf::test::scaffold::WithoutTestEnvironmentVariable> cleared_;
};

///
/// @brief Returns the path of the ecflow_server executable of the build under test.
///
inline fs::path ecflow_server_executable() {
    return fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server";
}

///
/// @brief Reserves a free local port, scanning upwards from the base port assigned to the server start-up tests.
///
/// @return The reserved port, locked for the lifetime of the returned object.
/// @throws std::runtime_error if ECF_TEST_SERVER_STARTUP_PORT_BASE is not set (it is set by the CMake test
/// definition).
///
inline ecf::test::scaffold::Port reserve_port() {
    const char* base = std::getenv("ECF_TEST_SERVER_STARTUP_PORT_BASE");
    if (base == nullptr) {
        throw std::runtime_error(
            "ECF_TEST_SERVER_STARTUP_PORT_BASE is not defined (it is set by the CMake test definition)");
    }
    const auto base_port = static_cast<ecf::test::scaffold::Port::port_t>(std::stoul(base));
    return ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{base_port}).create();
}

///
/// @brief Attempts a single TCP connection to the given local port.
///
/// @param[in] port Local port to connect to
/// @return The connection error; empty when a listener accepted the connection.
///
inline boost::system::error_code connect_once(std::uint32_t port) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    boost::system::error_code error;
    socket.connect({boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(port)}, error);
    return error;
}

///
/// @brief Polls the given local port until a listener accepts a connection, or the timeout expires.
///
/// @param[in] port Local port on which the server is expected to listen
/// @param[in] timeout Upper bound of the wait
/// @return The last connection error; empty when the server is listening.
///
inline boost::system::error_code
wait_for_listener(std::uint32_t port, std::chrono::seconds timeout = ecf::test::scaffold::server_startup_timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    boost::system::error_code error;
    do {
        error = connect_once(port);
        if (!error) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    } while (std::chrono::steady_clock::now() < deadline);
    return error;
}

///
/// @brief Reports whether the given local port refuses connections, as it does when no server listens on it.
///
/// @param[in] port Local port to probe
/// @return The connection error, compared by the caller with boost::asio::error::connection_refused.
///
inline boost::system::error_code probe_no_listener(std::uint32_t port) {
    return connect_once(port);
}

///
/// @brief Outcome of waiting for a launched server to exit.
///
struct ServerExit
{
    bool exited{false}; ///< True when the server exited by itself within the timeout
    int status{0};      ///< Exit status, or the terminating signal when the wait timed out
    std::string output; ///< Standard output followed by standard error
};

///
/// @brief Waits, with a bound, for a launched server to exit, terminating it when the timeout expires.
///
/// @param[in,out] process Launched server; reaped on return
/// @param[in] timeout Upper bound of the wait
/// @return Whether the server exited by itself, its exit status and its output.
///
inline ServerExit wait_for_exit(ecf::test::scaffold::Process& process,
                                std::chrono::seconds timeout = ecf::test::scaffold::server_startup_timeout) {
    auto completion = std::async(std::launch::async, [&process] { return process.wait(); });
    ServerExit result;
    result.exited = completion.wait_for(timeout) == std::future_status::ready;
    if (!result.exited) {
        process.terminate();
    }
    result.status = completion.get();
    result.output = process.read_stdout() + process.read_stderr();
    return result;
}

///
/// @brief Configures a client for a single, bounded connection attempt to a local test server.
///
/// @param[in,out] client Client to configure; errors are reported through return values, not exceptions
/// @param[in] host Server host name
/// @param[in] port Server port
///
inline void configure_client(ClientInvoker& client, const std::string& host, std::uint32_t port) {
    client.set_throw_on_error(false);
    client.set_host_port(host, std::to_string(port));
    client.set_connection_attempts(1);
    client.set_connect_timeout(std::chrono::seconds(5));
}

///
/// @brief Reads the exact bytes of a file owned by the test.
///
/// @param[in] path Readable file path
/// @return The complete file contents, including whitespace and final newlines.
///
inline std::string read_file_bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    BOOST_REQUIRE_MESSAGE(input.is_open(), "Unable to read test file: " << path);
    std::string result{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    BOOST_REQUIRE_MESSAGE(!input.bad(), "Unable to finish reading test file: " << path);
    return result;
}

} // namespace ecf::test::foolproof
