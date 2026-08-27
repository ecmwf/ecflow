/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/ServerProtocolProbe.hpp"

#include <algorithm>
#include <vector>

#include <boost/asio.hpp>

#ifdef ECF_OPENSSL
    #include <boost/asio/ssl.hpp>
#endif

#include "ecflow/base/Client.hpp"
#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/core/HttpLibrary.hpp"

namespace ecf {

bool probes_as_plain(const std::string& host, const std::string& port, std::chrono::milliseconds timeout) {
    try {
        boost::asio::io_context io;
        ConnectionDiagnosis diagnosis;
        Client probe(io, std::make_shared<CtsCmd>(CtsCmd::PING), host, port, timeout, &diagnosis);
        io.run();
        // Only a decodable reply proves the endpoint speaks the ecFlow protocol over plain TCP/IP.
        // Every other outcome is ambiguous, and a wrong answer is worse than no answer at all.
        return diagnosis.ok();
    }
    catch (...) {
        return false;
    }
}

bool probes_as_tls([[maybe_unused]] const std::string& host,
                   [[maybe_unused]] const std::string& port,
                   [[maybe_unused]] std::chrono::milliseconds timeout) {
#ifdef ECF_OPENSSL
    try {
        boost::asio::io_context io;

        boost::asio::ssl::context context(boost::asio::ssl::context::tls_client);
        context.set_verify_mode(boost::asio::ssl::verify_none);

        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(io, context);

        boost::system::error_code resolve_error;
        boost::asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host, port, resolve_error);
        if (resolve_error) {
            return false;
        }

        bool completed = false;
        bool finished  = false;

        boost::asio::system_timer deadline(io);
        deadline.expires_after(timeout);
        deadline.async_wait([&](const boost::system::error_code& error) {
            if (!error && !finished) {
                boost::system::error_code ignored;
                stream.lowest_layer().close(ignored);
            }
        });

        boost::asio::async_connect(
            stream.lowest_layer(), endpoints, [&](const boost::system::error_code& error, const auto&) {
                if (error) {
                    finished = true;
                    deadline.cancel();
                    return;
                }
                stream.async_handshake(boost::asio::ssl::stream_base::client,
                                       [&](const boost::system::error_code& handshake_error) {
                                           completed = !handshake_error;
                                           finished  = true;
                                           deadline.cancel();
                                       });
            });

        io.run();

        boost::system::error_code ignored;
        stream.lowest_layer().close(ignored);

        return completed;
    }
    catch (...) {
        return false;
    }
#else
    return false;
#endif
}

bool probes_as_http(const std::string& scheme,
                    const std::string& host,
                    const std::string& port,
                    std::chrono::milliseconds timeout) {
#ifndef ECF_OPENSSL
    if (scheme == "https") {
        return false;
    }
#endif
    try {
        httplib::Client client(scheme + "://" + host + ":" + port);
        client.set_connection_timeout(timeout);
        client.set_read_timeout(timeout);
        client.set_write_timeout(timeout);
#ifdef ECF_OPENSSL
        if (scheme == "https") {
            client.enable_server_certificate_verification(false);
        }
#endif
        // Any well-formed status line, including an error status, proves the endpoint speaks HTTP.
        auto result = client.Get("/");
        return static_cast<bool>(result);
    }
    catch (...) {
        return false;
    }
}

std::optional<Protocol> probe_server_protocol(const std::string& host,
                                              const std::string& port,
                                              std::chrono::milliseconds timeout,
                                              std::optional<Protocol> try_first) {

    auto speaks = [&](Protocol protocol) {
        switch (protocol) {
            case Protocol::Plain:
                return probes_as_plain(host, port, timeout);
            case Protocol::Http:
                return probes_as_http("http", host, port, timeout);
            case Protocol::Https:
                // An endpoint that completes a TLS handshake and then answers HTTP inside it is
                // serving HTTPS; the handshake alone would not tell HTTPS apart from plain SSL.
                return probes_as_tls(host, port, timeout) && probes_as_http("https", host, port, timeout);
            case Protocol::Ssl:
                return probes_as_tls(host, port, timeout) && !probes_as_http("https", host, port, timeout);
        }
        return false;
    };

    std::vector<Protocol> order = {Protocol::Plain, Protocol::Ssl, Protocol::Http, Protocol::Https};
    if (try_first) {
        order.erase(std::remove(order.begin(), order.end(), try_first.value()), order.end());
        order.insert(order.begin(), try_first.value());
    }

    for (auto protocol : order) {
        if (speaks(protocol)) {
            return protocol;
        }
    }

    return std::nullopt;
}

} // namespace ecf
