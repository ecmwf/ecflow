// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

// ecFlow client bridge — wraps `ClientInvoker`.
#pragma once

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/client/ClientInvoker.hpp"

#include "rust/cxx.h"

#include <cstdint>
#include <memory>

namespace ecflow_bridge {

//----------------------------------------------------------------------------------------------------------------------

/// Wraps `ClientInvoker` for Rust FFI.
///
/// Every request runs in the invoker's throw-on-error mode, the mode the
/// Python bindings use; cxx turns the exception into a `Result` on the Rust
/// side. The failure class of the request that threw is kept, so the Rust
/// error can carry it.
class ClientWrapper {
    ClientInvoker invoker_;
    ecf::ConnectionFailure failure_{ecf::ConnectionFailure::None};

public:

    ClientWrapper();
    ClientWrapper(rust::Str host, rust::Str port);

    // Connection configuration
    void set_host_port(rust::Str host, rust::Str port);
    rust::String host() const;
    rust::String port() const;
    void set_user_name(rust::Str user);
    void set_password(rust::Str password);
    void enable_ssl();
    void disable_ssl();
    void enable_http();
    void enable_https();
    void set_connect_timeout(uint64_t milliseconds);
    void set_retry_connection_period(uint64_t milliseconds);
    void set_connection_attempts(uint32_t attempts);
    void debug(bool enabled);

    // Failure class of the request that last threw (ecf::ConnectionFailure)
    int32_t last_failure() const;

    // Server probes
    void ping_server();
    rust::String server_version();
    rust::String stats();

    // Any command, as `ecflow_client` command line arguments
    rust::String invoke(rust::Slice<const rust::String> args);
    rust::Vec<rust::String> reply_strings() const;

    // Child (task) commands
    void set_child_path(rust::Str path);
    void set_child_password(rust::Str password);
    void set_child_pid(rust::Str pid);
    void set_child_try_no(uint32_t try_no);
    void set_child_timeout(uint32_t seconds);
    void set_zombie_child_timeout(uint32_t seconds);
    void child_init();
    void child_abort(rust::Str reason);
    void child_event(rust::Str name, bool value);
    void child_meter(rust::Str name, int32_t value);
    void child_label(rust::Str name, rust::Str value);
    void child_wait(rust::Str expression);
    rust::String child_queue(rust::Str queue, rust::Str action, rust::Str step, rust::Str path);
    void child_complete();

    // Definitions as text
    rust::String get_defs_text(int32_t style);
    void load_defs_text(rust::Str defs, bool force);
    void replace_text(rust::Str path, rust::Str defs, bool create_parents, bool force);

    // Access underlying for other C++ bridge code
    const ClientInvoker& inner() const { return invoker_; }
    ClientInvoker& inner() { return invoker_; }

    // ============== Factories ==============

    /// Create a client configured from the environment.
    static std::unique_ptr<ClientWrapper> create();

    /// Create a client for the given host and port.
    static std::unique_ptr<ClientWrapper> from_host_port(rust::Str host, rust::Str port);

private:

    /// Run a request, recording its failure class if it throws.
    template <typename F>
    auto request(F&& f) -> decltype(f());
};

//----------------------------------------------------------------------------------------------------------------------

/// The ecFlow version the library was built from.
rust::String version();

/// Whether the library was built with OpenSSL support.
bool ssl_supported();

//----------------------------------------------------------------------------------------------------------------------

}  // namespace ecflow_bridge
