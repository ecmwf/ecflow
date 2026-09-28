// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "EcflowBridge.h"

#include <algorithm>
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
    record_questions();
}

Client::Client(rust::Str host, rust::Str port)
    : ClientInvoker(std::string(host), std::string(port)) {
    enable_ssl_from_environment(*this);
    record_questions();
}

void Client::record_questions() {
    set_confirmation_handler([this](const ecf::Confirmation& what) {
        const bool approved = std::any_of(approved_.begin(), approved_.end(), [&what](const ecf::Confirmation& it) {
            return it.command == what.command && it.paths == what.paths;
        });
        questions_.push_back(Asked{what, approved});
        return approved;
    });
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
    questions_.clear();
    std::vector<std::string> argv;
    argv.reserve(args.size() + 1);
    argv.emplace_back("ecflow_client"); // argv[0], as the command line parser expects
    for (const auto& arg : args) {
        argv.emplace_back(std::string(arg));
    }
    ClientInvoker::invoke(argv);
}

void Client::delete_nodes(rust::Slice<const rust::String> paths, bool force) const {
    std::vector<std::string> nodes;
    nodes.reserve(paths.size());
    for (const auto& path : paths) {
        nodes.emplace_back(std::string(path));
    }
    ClientInvoker::delete_nodes(nodes, force);
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

uint64_t Client::round_trip_time() const {
    return static_cast<uint64_t>(ClientInvoker::round_trip_time().total_microseconds());
}

rust::Vec<Question> Client::questions() const {
    rust::Vec<Question> result;
    for (const auto& asked : questions_) {
        Question question;
        question.command = rust::String(asked.what.command);
        for (const auto& path : asked.what.paths) {
            question.paths.push_back(rust::String(path));
        }
        question.approved = asked.approved;
        result.push_back(std::move(question));
    }
    return result;
}

void Client::approve(const Question& question) {
    ecf::Confirmation what;
    what.command = std::string(question.command);
    for (const auto& path : question.paths) {
        what.paths.emplace_back(std::string(path));
    }
    approved_.push_back(std::move(what));
}

void Client::forget_approvals() {
    approved_.clear();
}

} // namespace ecflow_bridge
