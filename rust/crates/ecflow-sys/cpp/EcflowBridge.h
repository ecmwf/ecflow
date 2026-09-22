// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

// ecFlow C++ bridge for Rust FFI.
#pragma once

#include <cstdint>
#include <memory>

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "rust/cxx.h"

namespace ecflow_bridge {

/// The print style of definitions written as text; the bridge binds it as an enum of this namespace.
using DefsStyle = ::PrintStyle::Type_t;

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

    /// Run any command given as `ecflow_client` command line arguments.
    void invoke(rust::Slice<const rust::String> args) const;

    /// The list of strings in the most recent reply, for commands that return one.
    rust::Vec<rust::String> reply_strings() const;

    /// The base method returns `std::string` by value, which cxx cannot bind.
    rust::String
    child_queue(const std::string& queue, const std::string& action, const std::string& step, const std::string& path);

    /// Fetch the server's definitions and write them as text.
    rust::String defs_text(DefsStyle style) const;

    /// The failure class of the most recent request.
    ecf::ConnectionFailure last_failure() const;

private:
    Client();
    Client(rust::Str host, rust::Str port);
};

} // namespace ecflow_bridge
