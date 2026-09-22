// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "EcflowBridge.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecflow-sys/src/lib.rs.h"
#include "ecflow/core/Version.hpp"

namespace ecflow_bridge {

namespace {

/// Mirror the Python bindings: SSL is enabled when ECF_SSL is set, since the
/// invoker constructor loads every environment variable except that one.
void enable_ssl_from_environment(ClientInvoker& client) {
#if defined(ECF_OPENSSL)
    if (std::getenv("ECF_SSL") != nullptr) {
        client.enable_ssl_if_defined();
    }
#else
    (void)client;
#endif
}

} // namespace

Client::Client()
    : ClientInvoker() {
    enable_ssl_from_environment(*this);
}

Client::Client(rust::Str host, rust::Str port)
    : ClientInvoker(std::string(host), std::string(port)) {
    enable_ssl_from_environment(*this);
}

std::unique_ptr<Client> Client::create() {
    return std::unique_ptr<Client>(new Client());
}

std::unique_ptr<Client> Client::from_host_port(rust::Str host, rust::Str port) {
    return std::unique_ptr<Client>(new Client(host, port));
}

std::shared_ptr<Defs> Client::parse_defs(rust::Str text) {
    defs_ptr defs = Defs::create();
    std::string error;
    std::string warning;
    if (!defs->restore_from_string(std::string(text), error, warning)) {
        throw std::runtime_error(error);
    }
    return defs;
}

rust::String Client::version() {
    return rust::String(ecf::Version::full());
}

bool Client::ssl_supported() {
#if defined(ECF_OPENSSL)
    return true;
#else
    return false;
#endif
}

void Client::set_connect_timeout(uint64_t milliseconds) {
    ClientInvoker::set_connect_timeout(std::chrono::milliseconds{milliseconds});
}

void Client::set_retry_connection_period(uint64_t milliseconds) {
    ClientInvoker::set_retry_connection_period(std::chrono::milliseconds{milliseconds});
}

void Client::enable_ssl() {
#if defined(ECF_OPENSSL)
    ClientInvoker::enable_ssl();
#else
    throw std::runtime_error("ecflow-sys was built without the ssl feature");
#endif
}

void Client::disable_ssl() {
#if defined(ECF_OPENSSL)
    ClientInvoker::disable_ssl();
#endif
}

void Client::invoke(rust::Slice<const rust::String> args) const {
    std::vector<std::string> argv;
    argv.reserve(args.size() + 1);
    argv.emplace_back("ecflow_client"); // argv[0], as the command line parser expects
    for (const auto& arg : args) {
        argv.emplace_back(std::string(arg));
    }
    ClientInvoker::invoke(argv);
}

rust::Vec<rust::String> Client::reply_strings() const {
    rust::Vec<rust::String> result;
    const auto& strings = server_reply().get_string_vec();
    result.reserve(strings.size());
    for (const auto& s : strings) {
        result.push_back(rust::String(s));
    }
    return result;
}

rust::String Client::child_queue(const std::string& queue,
                                 const std::string& action,
                                 const std::string& step,
                                 const std::string& path) {
    return rust::String(ClientInvoker::child_queue(queue, action, step, path));
}

rust::String Client::defs_text(DefsStyle style) const {
    getDefs();
    defs_ptr defs = ClientInvoker::defs();
    if (!defs) {
        throw std::runtime_error("The server returned no definitions");
    }
    std::string text;
    defs->write_to_string(text, style);
    return rust::String(text);
}

ecf::ConnectionFailure Client::last_failure() const {
    return connection_diagnosis().failure;
}

} // namespace ecflow_bridge
