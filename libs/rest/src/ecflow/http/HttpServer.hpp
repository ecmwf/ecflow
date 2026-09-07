/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_HttpServer_HPP
#define ecflow_http_HttpServer_HPP

#include <functional>

namespace ecf::http {

class HttpServer {
public:
    ///
    /// @brief Notification issued once the server has acquired the port and is able to accept connections.
    ///
    /// The notification is issued from the thread that calls run(), before the accept loop is entered.
    ///
    using BoundCallback = std::function<void()>;

    HttpServer(int argc, char** argv);

    ///
    /// @brief Runs the server, until it is shut down.
    ///
    /// @param[in] on_bound Notification issued once the port has been acquired; not called if the port
    /// cannot be acquired. May be empty, in which case no notification is issued.
    /// @throws std::runtime_error if the server cannot be brought into a usable state (e.g., when the certificate and
    /// private key cannot be loaded), if the port cannot be acquired, or if the server stops unexpectedly.
    ///
    void run(const BoundCallback& on_bound = {}) const;

    ~HttpServer() = default;

private:
    void parse_args(int argc, char** argv) const;
};

} // namespace ecf::http

#endif /* ecflow_http_HttpServer_HPP */
