// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

// ecFlow client bridge — implementation.

#include "ClientWrapper.h"
#include "ecflow-sys/src/lib.rs.h"

#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Defs.hpp"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ecflow_bridge {

namespace {

std::string to_std(rust::Str s) {
    return std::string(s);
}

/// Mirror the Python bindings: SSL is enabled when ECF_SSL is set, since the
/// invoker constructor loads every environment variable except that one.
void enable_ssl_from_environment(ClientInvoker& invoker) {
#if defined(ECF_OPENSSL)
    if (std::getenv("ECF_SSL") != nullptr) {
        invoker.enable_ssl_if_defined();
    }
#else
    (void)invoker;
#endif
}

/// Parse a definition in the ecFlow text format.
defs_ptr parse_defs(rust::Str text) {
    defs_ptr defs = Defs::create();
    std::string error;
    std::string warning;
    if (!defs->restore_from_string(to_std(text), error, warning)) {
        throw std::runtime_error(error);
    }
    return defs;
}

}  // namespace

//----------------------------------------------------------------------------------------------------------------------

template <typename F>
auto ClientWrapper::request(F&& f) -> decltype(f()) {
    failure_ = ecf::ConnectionFailure::None;
    try {
        return f();
    }
    catch (...) {
        failure_ = invoker_.connection_diagnosis().failure;
        throw;
    }
}

ClientWrapper::ClientWrapper() : invoker_() {
    enable_ssl_from_environment(invoker_);
}

ClientWrapper::ClientWrapper(rust::Str host, rust::Str port) : invoker_(to_std(host), to_std(port)) {
    enable_ssl_from_environment(invoker_);
}

// Connection configuration

void ClientWrapper::set_host_port(rust::Str host, rust::Str port) {
    invoker_.set_host_port(to_std(host), to_std(port));
}

rust::String ClientWrapper::host() const {
    return rust::String(invoker_.host());
}

rust::String ClientWrapper::port() const {
    return rust::String(invoker_.port());
}

void ClientWrapper::set_user_name(rust::Str user) {
    invoker_.set_user_name(to_std(user));
}

void ClientWrapper::set_password(rust::Str password) {
    invoker_.set_password(to_std(password));
}

void ClientWrapper::enable_ssl() {
#if defined(ECF_OPENSSL)
    invoker_.enable_ssl();
#else
    throw std::runtime_error("ecflow-sys was built without the ssl feature");
#endif
}

void ClientWrapper::disable_ssl() {
#if defined(ECF_OPENSSL)
    invoker_.disable_ssl();
#endif
}

void ClientWrapper::enable_http() {
    invoker_.enable_http();
}

void ClientWrapper::enable_https() {
    invoker_.enable_https();
}

void ClientWrapper::set_connect_timeout(uint64_t milliseconds) {
    invoker_.set_connect_timeout(std::chrono::milliseconds{milliseconds});
}

void ClientWrapper::set_retry_connection_period(uint64_t milliseconds) {
    invoker_.set_retry_connection_period(std::chrono::milliseconds{milliseconds});
}

void ClientWrapper::set_connection_attempts(uint32_t attempts) {
    invoker_.set_connection_attempts(attempts);
}

void ClientWrapper::debug(bool enabled) {
    invoker_.debug(enabled);
}

ecf::ConnectionFailure ClientWrapper::last_failure() const {
    return failure_;
}

// Server probes

void ClientWrapper::ping_server() {
    request([&] { invoker_.pingServer(); });
}

rust::String ClientWrapper::server_version() {
    request([&] { invoker_.server_version(); });
    return rust::String(invoker_.get_string());
}

rust::String ClientWrapper::stats() {
    request([&] { invoker_.stats(); });
    return rust::String(invoker_.get_string());
}

// Any command

rust::String ClientWrapper::invoke(rust::Slice<const rust::String> args) {
    std::vector<std::string> argv;
    argv.reserve(args.size() + 1);
    argv.emplace_back("ecflow_client");  // argv[0], as the command line parser expects
    for (const auto& arg : args) {
        argv.emplace_back(std::string(arg));
    }
    request([&] { invoker_.invoke(argv); });
    return rust::String(invoker_.get_string());
}

rust::Vec<rust::String> ClientWrapper::reply_strings() const {
    rust::Vec<rust::String> result;
    const auto& strings = invoker_.server_reply().get_string_vec();
    result.reserve(strings.size());
    for (const auto& s : strings) {
        result.push_back(rust::String(s));
    }
    return result;
}

// Child (task) commands

void ClientWrapper::set_child_path(rust::Str path) {
    invoker_.set_child_path(to_std(path));
}

void ClientWrapper::set_child_password(rust::Str password) {
    invoker_.set_child_password(to_std(password));
}

void ClientWrapper::set_child_pid(rust::Str pid) {
    invoker_.set_child_pid(to_std(pid));
}

void ClientWrapper::set_child_try_no(uint32_t try_no) {
    invoker_.set_child_try_no(try_no);
}

void ClientWrapper::set_child_timeout(uint32_t seconds) {
    invoker_.set_child_timeout(seconds);
}

void ClientWrapper::set_zombie_child_timeout(uint32_t seconds) {
    invoker_.set_zombie_child_timeout(seconds);
}

void ClientWrapper::child_init() {
    request([&] { invoker_.child_init(); });
}

void ClientWrapper::child_abort(rust::Str reason) {
    request([&] { invoker_.child_abort(to_std(reason)); });
}

void ClientWrapper::child_event(rust::Str name, bool value) {
    request([&] { invoker_.child_event(to_std(name), value); });
}

void ClientWrapper::child_meter(rust::Str name, int32_t value) {
    request([&] { invoker_.child_meter(to_std(name), value); });
}

void ClientWrapper::child_label(rust::Str name, rust::Str value) {
    request([&] { invoker_.child_label(to_std(name), to_std(value)); });
}

void ClientWrapper::child_wait(rust::Str expression) {
    request([&] { invoker_.child_wait(to_std(expression)); });
}

rust::String ClientWrapper::child_queue(rust::Str queue, rust::Str action, rust::Str step, rust::Str path) {
    return rust::String(
        request([&] { return invoker_.child_queue(to_std(queue), to_std(action), to_std(step), to_std(path)); }));
}

void ClientWrapper::child_complete() {
    request([&] { invoker_.child_complete(); });
}

// Definitions as text

rust::String ClientWrapper::get_defs_text(DefsStyle style) {
    request([&] { invoker_.getDefs(); });
    defs_ptr defs = invoker_.defs();
    if (!defs) {
        throw std::runtime_error("The server returned no definitions");
    }
    std::string text;
    defs->write_to_string(text, style);
    return rust::String(text);
}

void ClientWrapper::load_defs_text(rust::Str text, bool force) {
    defs_ptr defs = parse_defs(text);
    request([&] { invoker_.load(defs, force); });
}

void ClientWrapper::replace_text(rust::Str path, rust::Str text, bool create_parents, bool force) {
    defs_ptr defs = parse_defs(text);
    request([&] { invoker_.replace_1(to_std(path), defs, create_parents, force); });
}

// Factories

std::unique_ptr<ClientWrapper> ClientWrapper::create() {
    return std::make_unique<ClientWrapper>();
}

std::unique_ptr<ClientWrapper> ClientWrapper::from_host_port(rust::Str host, rust::Str port) {
    return std::make_unique<ClientWrapper>(host, port);
}

//----------------------------------------------------------------------------------------------------------------------

rust::String version() {
    return rust::String(ecf::Version::full());
}

bool ssl_supported() {
#if defined(ECF_OPENSSL)
    return true;
#else
    return false;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace ecflow_bridge
