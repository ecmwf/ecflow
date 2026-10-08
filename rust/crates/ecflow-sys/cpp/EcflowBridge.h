// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

// ecFlow C++ bridge for Rust FFI.
#pragma once

#include <cstdint>
#include <memory>

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/NOrder.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "rust/cxx.h"

namespace ecflow_bridge {

struct HandleSuites;
struct NameValue;
struct Zombie;

/// The print style of definitions written as text; the bridge binds it as an enum of this namespace.
using DefsStyle = ::PrintStyle::Type_t;

/// Where a node moves among its siblings, or how they are sorted; the bridge binds it as an enum of this namespace.
using NodeOrder = ::NOrder::Order;

/// When the server writes its check point file; the bridge binds it as an enum of this namespace.
using CheckPtMode = ::ecf::CheckPt::Mode;

/// The state of a node; the bridge binds it as an enum of this namespace.
using NodeState = ::NState::State;

/// A `ClientInvoker` with the members cxx cannot bind on the base class:
/// constructors, `std::chrono` arguments, `ECF_OPENSSL` guards, argument
/// vectors and strings returned by value.
///
/// Every request runs in the invoker's throw-on-error mode, the mode the
/// Python bindings use; cxx turns the exception into a `Result` on the Rust
/// side.
class Client : public ClientInvoker {
public:
    /// Create a client configured from the environment; SSL is enabled when
    /// ECF_SSL is set, as the Python bindings do.
    static std::unique_ptr<Client> create();

    /// Create a client for the given host and port.
    static std::unique_ptr<Client> from_host_port(rust::Str host, rust::Str port);

    /// Parse definitions given in the ecFlow text format.
    static std::shared_ptr<Defs> parse_defs(rust::Str text);

    /// The ecFlow version the library was built from.
    static rust::String version();

    /// Whether the library was built with OpenSSL support.
    static bool ssl_supported();

    void set_connect_timeout(uint64_t milliseconds);
    void set_retry_connection_period(uint64_t milliseconds);
    void enable_ssl();
    void disable_ssl();

    /// The base methods take `std::vector<std::string>`, which cxx cannot build from Rust.
    void delete_nodes(rust::Slice<const rust::String> paths, bool force) const;
    void suspend(rust::Slice<const rust::String> paths) const;
    void resume(rust::Slice<const rust::String> paths) const;
    void requeue(rust::Slice<const rust::String> paths, const std::string& option) const;
    void run(rust::Slice<const rust::String> paths, bool force) const;
    void kill(rust::Slice<const rust::String> paths) const;
    void status(rust::Slice<const rust::String> paths) const;
    void check(rust::Slice<const rust::String> paths) const;
    void archive(rust::Slice<const rust::String> paths, bool force) const;
    void restore(rust::Slice<const rust::String> paths) const;
    void
    force(rust::Slice<const rust::String> paths, NodeState state, bool recursive, bool set_repeats_to_last_value) const;
    void force_event(rust::Slice<const rust::String> paths, bool set) const;
    void freeDep(rust::Slice<const rust::String> paths, bool trigger, bool all, bool date, bool time) const;
    void alter(rust::Slice<const rust::String> paths,
               const std::string& alter_type,
               const std::string& attr_type,
               const std::string& name,
               const std::string& value) const;
    void alter_sort(rust::Slice<const rust::String> paths, const std::string& attribute, bool recursive) const;
    void zombieFobCliPaths(rust::Slice<const rust::String> paths) const;
    void zombieFailCliPaths(rust::Slice<const rust::String> paths) const;
    void zombieAdoptCliPaths(rust::Slice<const rust::String> paths) const;
    void zombieBlockCliPaths(rust::Slice<const rust::String> paths) const;
    void zombieRemoveCliPaths(rust::Slice<const rust::String> paths) const;
    void zombieKillCliPaths(rust::Slice<const rust::String> paths) const;
    void ch_register(bool auto_add_new_suites, rust::Slice<const rust::String> suites) const;
    void ch_add(int client_handle, rust::Slice<const rust::String> suites) const;
    void ch_remove(int client_handle, rust::Slice<const rust::String> suites) const;

    /// The registered handles and their suites, from the most recent `ch_suites` reply.
    rust::Vec<HandleSuites> client_handle_suites() const;

    /// The zombies of the most recent `zombieGet` reply.
    rust::Vec<Zombie> zombies() const;

    /// The base methods take `NameValueVec` and `std::vector<std::string>`, which cxx cannot build from Rust.
    void edit_script_preprocess(const std::string& path, rust::Slice<const rust::String> file_contents);
    void edit_script_submit(const std::string& path,
                            rust::Slice<const NameValue> used_variables,
                            rust::Slice<const rust::String> file_contents,
                            bool alias,
                            bool run);
    void set_child_init_add_vars(rust::Slice<const NameValue> vars);
    void set_child_complete_del_vars(rust::Slice<const rust::String> names);

    /// The list of strings in the most recent reply, for commands that return one.
    rust::Vec<rust::String> reply_strings() const;

    /// The base method returns `std::string` by value, which cxx cannot bind.
    rust::String
    child_queue(const std::string& queue, const std::string& action, const std::string& step, const std::string& path);

    /// Write the definition the client holds as text.
    rust::String defs_text(DefsStyle style) const;

    /// The base method returns `std::string` by value, which cxx cannot bind.
    rust::String get_certificate() const;

    /// The failure class of the most recent request.
    ecf::ConnectionFailure last_failure() const;

    /// The round trip time of the most recent request, in microseconds; the
    /// base method returns a Boost duration, which cxx cannot bind.
    uint64_t round_trip_time() const;

private:
    Client();
    Client(rust::Str host, rust::Str port);
};

} // namespace ecflow_bridge
