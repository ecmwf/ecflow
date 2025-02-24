/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Openssl_HPP
#define ecflow_base_Openssl_HPP

#include <memory>
#include <ostream>
#include <string>

#include <boost/asio/ssl.hpp>

namespace ecf {

class Openssl {
public:
    Openssl(const Openssl&)                  = delete;
    const Openssl& operator=(const Openssl&) = delete;

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

    const std::string& ssl() const { return ssl_; }
    bool enabled() const { return !ssl_.empty(); }

    bool enable_no_throw(std::string host,
                         const std::string& port,
                         const std::string& ecf_ssl_env = "");         // returns false if cannot enable
    void enable(std::string host, const std::string& port);            // Can throw if certificate (CRT) are missing
    void enable_if_defined(std::string host, const std::string& port); // Can throw if certificate (CRT) are missing
    void disable() { ssl_.clear(); }                                   // override environment setting for ECF_SSL

    void init_for_server();
    void init_for_client();

    boost::asio::ssl::context& context();

    static const char* ssl_info();
    void print(std::ostream& os) const { os << ssl_; }
    std::string info() const;

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

#endif /* ecflow_base_Openssl_HPP */
