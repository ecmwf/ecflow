// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <ostream>
#include <string>

#include <boost/asio/ssl.hpp>

namespace ecf {

class Openssl {
public:
    /// There is no SSL protocol version named SSLv23.
    /// The SSLv23_method() API and its variants choose SSLv2, SSLv3, or TLSv1 for compatibility with the peer
    /// Consider the incompatibility among the SSL/TLS versions when you develop
    /// SSL client/server applications. For example, a TLSv1 server cannot understand
    /// a client-hello message from an SSLv2 or SSLv3 client.
    /// The SSLv2 client/server recognises messages from only an SSLv2 peer.
    /// The SSLv23_method() API and its variants may be used when the compatibility with the peer is important.
    /// An SSL server with the SSLv23 method can understand any of the SSLv2, SSLv3, and TLSv1 hello messages.
    /// However, the SSL client using the SSLv23 method cannot establish connection with the SSL server
    ///  with the SSLv3/TLSv1 method because SSLv2 hello message is sent by the client
    Openssl() = default;

    Openssl(const Openssl&)            = delete;
    Openssl& operator=(const Openssl&) = delete;
    Openssl(Openssl&&)                 = delete;
    Openssl& operator=(Openssl&&)      = delete;

    ~Openssl() = default;

    const std::string& ssl() const { return ssl_; }
    bool enabled() const { return !ssl_.empty(); }

    ///
    /// @brief Enables SSL using shared-first or specific-first certificate selection.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @param[in] ecf_ssl_env Empty or "1" selects shared-first; any other value selects specific-first.
    /// @return True if either certificate exists; false if neither exists, leaving SSL disabled.
    /// @throws fs::filesystem_error if a certificate lookup fails.
    ///
    bool enable_no_throw(std::string host, const std::string& port, const std::string& ecf_ssl_env = "");

    void enable(std::string host, const std::string& port);            // Can throw if certificate (CRT) are missing
    void enable_if_defined(std::string host, const std::string& port); // Can throw if certificate (CRT) are missing
    void disable() { ssl_.clear(); }                                   // override environment setting for ECF_SSL

    ///
    /// @brief Enables SSL preferring the server-specific certificate, independently of ECF_SSL.
    ///
    /// Uses the same certificate selection as ECF_SSL=X, falling back to server.crt only if absent.
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @throws std::runtime_error if neither the server-specific nor shared certificate file exists.
    ///
    void enable_prefer_specific(std::string host, const std::string& port);

    void init_for_server();
    void init_for_client();

    boost::asio::ssl::context& context();

    static const char* ssl_info();
    void print(std::ostream& os) const { os << ssl_; }

    ///
    /// @brief Describes the selected SSL certificate for diagnostic logging.
    ///
    /// @return The absolute certificate path and its shared or server-specific category, or "SSL disabled".
    /// @throws std::filesystem::filesystem_error if the absolute certificate path cannot be determined.
    ///
    std::string info() const;

    std::string selected_crt() const;

private:
    void check_server_certificates() const;
    void load_verify_file(boost::asio::ssl::context&); /// load server.crt file into the ssl context
    std::string certificates_dir() const;              /// Directory where ssl certificates are held
    std::string get_password() const;

    std::string pem() const;
    std::string crt() const;
    std::string key() const;
    std::string passwd() const;

private:
    std::string ssl_;                                        // Non-empty if ssl has been enabled
    std::unique_ptr<boost::asio::ssl::context> ssl_context_; // create on demand, otherwise non-ssl context takes a hit.
    bool init_for_client_{false};
};

std::ostream& operator<<(std::ostream& os, const ecf::Openssl&);

} // namespace ecf
