// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "EcflowBridge.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecflow-sys/src/lib.rs.h"
#include "ecflow/attribute/NodeAttr.hpp"
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

/// The `std::vector<std::string>` the base methods take.
std::vector<std::string> strings(rust::Slice<const rust::String> items) {
    std::vector<std::string> result;
    result.reserve(items.size());
    for (const auto& item : items) {
        result.emplace_back(std::string(item));
    }
    return result;
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

void Client::delete_nodes(rust::Slice<const rust::String> paths, bool force) const {
    ClientInvoker::delete_nodes(strings(paths), force);
}

void Client::suspend(rust::Slice<const rust::String> paths) const {
    ClientInvoker::suspend(strings(paths));
}

void Client::resume(rust::Slice<const rust::String> paths) const {
    ClientInvoker::resume(strings(paths));
}

void Client::requeue(rust::Slice<const rust::String> paths, const std::string& option) const {
    ClientInvoker::requeue(strings(paths), option);
}

void Client::run(rust::Slice<const rust::String> paths, bool force) const {
    ClientInvoker::run(strings(paths), force);
}

void Client::kill(rust::Slice<const rust::String> paths) const {
    ClientInvoker::kill(strings(paths));
}

void Client::status(rust::Slice<const rust::String> paths) const {
    ClientInvoker::status(strings(paths));
}

void Client::check(rust::Slice<const rust::String> paths) const {
    ClientInvoker::check(strings(paths));
}

void Client::archive(rust::Slice<const rust::String> paths, bool force) const {
    ClientInvoker::archive(strings(paths), force);
}

void Client::restore(rust::Slice<const rust::String> paths) const {
    ClientInvoker::restore(strings(paths));
}

void Client::force(rust::Slice<const rust::String> paths,
                   NodeState state,
                   bool recursive,
                   bool set_repeats_to_last_value) const {
    ClientInvoker::force(strings(paths), NState::toString(state), recursive, set_repeats_to_last_value);
}

void Client::force_event(rust::Slice<const rust::String> paths, bool set) const {
    ClientInvoker::force(strings(paths), set ? Event::SET() : Event::CLEAR());
}

void Client::freeDep(rust::Slice<const rust::String> paths, bool trigger, bool all, bool date, bool time) const {
    ClientInvoker::freeDep(strings(paths), trigger, all, date, time);
}

void Client::alter(rust::Slice<const rust::String> paths,
                   const std::string& alter_type,
                   const std::string& attr_type,
                   const std::string& name,
                   const std::string& value) const {
    ClientInvoker::alter(strings(paths), alter_type, attr_type, name, value);
}

void Client::alter_sort(rust::Slice<const rust::String> paths, const std::string& attribute, bool recursive) const {
    ClientInvoker::alter_sort(strings(paths), attribute, recursive);
}

void Client::zombieFobCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieFobCliPaths(strings(paths));
}

void Client::zombieFailCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieFailCliPaths(strings(paths));
}

void Client::zombieAdoptCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieAdoptCliPaths(strings(paths));
}

void Client::zombieBlockCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieBlockCliPaths(strings(paths));
}

void Client::zombieRemoveCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieRemoveCliPaths(strings(paths));
}

void Client::zombieKillCliPaths(rust::Slice<const rust::String> paths) const {
    ClientInvoker::zombieKillCliPaths(strings(paths));
}

void Client::ch_register(bool auto_add_new_suites, rust::Slice<const rust::String> suites) const {
    ClientInvoker::ch_register(auto_add_new_suites, strings(suites));
}

void Client::ch_add(int client_handle, rust::Slice<const rust::String> suites) const {
    ClientInvoker::ch_add(client_handle, strings(suites));
}

void Client::ch_remove(int client_handle, rust::Slice<const rust::String> suites) const {
    ClientInvoker::ch_remove(client_handle, strings(suites));
}

rust::Vec<HandleSuites> Client::client_handle_suites() const {
    rust::Vec<HandleSuites> result;
    for (const auto& [handle, suites] : server_reply().get_client_handle_suites()) {
        HandleSuites item;
        item.handle = static_cast<int>(handle);
        for (const auto& suite : suites) {
            item.suites.push_back(rust::String(suite));
        }
        result.push_back(std::move(item));
    }
    return result;
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

} // namespace ecflow_bridge
