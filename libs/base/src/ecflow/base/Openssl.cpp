/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/Openssl.hpp"

#include <cassert>
#include <stdexcept>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;

namespace ecf {

std::string Openssl::info() const {
    if (ssl_ == "1")
        return "1 : enabled : uses shared ssl certificates";
    return ssl_ + " : enabled : uses server/port specific ssl certificates";
}

bool Openssl::enable_no_throw(std::string host, const std::string& port, const std::string& ecf_ssl_env) {
    if (host == Str::LOCALHOST())
        host = Host().name();

    //   if (Ecf::server()) cout << "Openssl::enable(SERVER) ---> input host:" << host << " port:" << port << " ECF_SSL
    //   = " << ecf_ssl_env << "\n"; else               cout << "Openssl::enable(CLIENT) ---> input host:" << host << "
    //   port:" << port << " ECF_SSL = " << ecf_ssl_env << "\n";

    if (ecf_ssl_env.empty() || ecf_ssl_env == "1") {
        // LOOK for      $HOME/.ecflowrc/ssl/server.crt
        // THEN LOOK for $HOME/.ecflowrc/ssl/<host>.<port>.crt
        // Look for the certificate that HAVE to exist on both client and server. i.e Self signed certificate (CRT)
        // Needed for testing, avoid <host>.<port>.crt when ports numbers are auto generated
        ssl_ = "1";
        if (!fs::exists(crt())) { // crt() uses ssl_

            // More specific per server. But we take an extra hit on file access.
            ssl_ = host;
            ssl_ += ".";
            ssl_ += port;
            if (!fs::exists(crt())) { // crt() uses ssl_
                ssl_.clear();
                return false;
            }
        }
    }
    else {
        ssl_ = host;
        ssl_ += ".";
        ssl_ += port;
        if (!fs::exists(crt())) { // crt() uses ssl_
            ssl_.clear();
            return false;
        }
    }
    // cout << "Openssl::enable input ssl_:'" << ssl_ << "'\n";
    return true;
}

void Openssl::enable(std::string host, const std::string& port) {
    if (host == Str::LOCALHOST())
        host = Host().name();

    if (!enable_no_throw(host, port)) {
        std::stringstream ss;
        ss << "Openssl::enable: Error: Expected to find the self signed certificate file(CRT) server.crt or " << host
           << "." << port << ".crt in $HOME/.ecflowrc/ssl";
        throw std::runtime_error(ss.str());
    }
}

void Openssl::enable_if_defined(std::string host, const std::string& port) {
    if (auto ecf_ssl = ecf::environment::fetch<std::string>(ecf::environment::ECF_SSL); ecf_ssl) {
        std::string ecf_ssl_env = ecf_ssl.value();

        if (host == Str::LOCALHOST())
            host = Host().name();

        if (!enable_no_throw(host, port, ecf_ssl_env)) {
            std::stringstream ss;
            if (ecf_ssl_env == "1") {
                ss << "Openssl::enable: Error: Expected to find the self signed certificate file(CRT) server.crt *OR* "
                   << host << "." << port << ".crt in $HOME/.ecflowrc/ssl when ECF_SSL=1";
            }
            else {
                ss << "Openssl::enable: Error: Expected to find the self signed certificate file(CRT) " << host << "."
                   << port << ".crt in $HOME/.ecflowrc/ssl when ECF_SSL=" << host << "." << port;
            }
            throw std::runtime_error(ss.str());
        }
    }
}

boost::asio::ssl::context& Openssl::context() {
    assert(ssl_context_);
    return *ssl_context_;
}

void Openssl::init_for_server() {
    //   std::cout << " Openssl::init_for_server  host :" << host_ << "@" << port_ << "\n";
    //   std::cout << " Openssl::init_for_server ssl_:'" << ssl_ << "'\n";
    if (enabled()) {
        check_server_certificates();

        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
        ssl_context_->set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                                  boost::asio::ssl::context::single_dh_use);
        // this must be done before loading any keys. as below
        ssl_context_->set_password_callback(
            [this](std::size_t size, boost::asio::ssl::context_base::password_purpose purpose) {
                return this->get_password();
            });
        ssl_context_->use_certificate_chain_file(crt());
        ssl_context_->use_private_key_file(key(), boost::asio::ssl::context::pem);
        ssl_context_->use_tmp_dh_file(pem());
    }
}

void Openssl::init_for_client() {
    //   std::cout << " Openssl::init_for_client host :" << host_ << "@" << port_ << "\n";
    if (!init_for_client_ && enabled()) {
        init_for_client_ = true;

        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
        ssl_context_->load_verify_file(crt());
    }
}

std::string Openssl::get_password() const {
    // std::cout << "Openssl::get_password()\n";
    std::string passwd_file = passwd();
    if (fs::exists(passwd_file)) {
        std::string contents;
        if (ecf::File::open(passwd_file, contents)) {
            // remove /n added by editor.
            if (!contents.empty() && contents[contents.size() - 1] == '\n')
                contents.erase(contents.begin() + contents.size() - 1);
            // std::cout << "Server::get_password() passwd('" << contents << "')\n";
            return contents;
        }
        else {
            std::stringstream ss;
            ss << "Server::get_password file " << passwd_file << " exists, but can't be opened (" << strerror(errno)
               << ")";
            throw std::runtime_error(ss.str());
        }
    }
    // std::cout << "Server::get_password() passwd('test')\n";
    return "test";
}

std::string Openssl::certificates_dir() const {
    if (auto found = ecf::environment::fetch<std::string>("ECF_SSL_DIR"); found) {
        // This is used for testing, to avoid using the default location
        return found.value();
    }
    else {
        return ecf::environment::get<std::string>("HOME") + "/.ecflowrc/ssl/";
    }
}

std::string Openssl::pem() const {
    std::string str = certificates_dir();
    if (ssl_ == "1") {
        // assume 2048-bit dh key, but accept 1024-bit for
        // backwards-compatibility (note: openssl might
        // not accept it, depending on the version used)
        str += "dh2048.pem";

        if (!fs::exists(str)) {
            return certificates_dir() + "dh1024.pem";
        }
        return str;
    }

    str += ssl_;
    str += ".pem";
    return str;
}

std::string Openssl::crt() const {
    std::string str = certificates_dir();
    if (ssl_ == "1") {
        str += "server.crt";
        return str;
    }
    str += ssl_;
    str += ".crt";
    return str;
}

std::string Openssl::key() const {
    std::string str = certificates_dir();
    if (ssl_ == "1") {
        str += "server.key";
        return str;
    }
    str += ssl_;
    str += ".key";
    return str;
}

std::string Openssl::passwd() const {
    std::string str = certificates_dir();
    if (ssl_ == "1") {
        str += "server.passwd";
        return str;
    }
    str += ssl_;
    str += ".passwd";
    return str;
}

const char* Openssl::ssl_info() {
    return "ecFlow client and server are SSL enabled. To use SSL choose between:\n"
           "  1. export ECF_SSL=1              # search for server.crt otherwise <host>.<port>.crt\n"
           "  2. export ECF_SSL=<host>.<port>  # Use server specific certificates <host>.<port>.***\n"
           "  3. use --ssl           # argument on ecflow_client/ecflow_server, same as option 1.\n"
           "                         # Typically ssl server can be started with ecflow_start.sh -s\n"
           "  4. Client.enable_ssl() # for python client\n\n"
           "ecFlow expects the certificates to be in directory $HOME/.ecflowrc/ssl\n"
           "The certificates can be shared if you have multiple servers running on\n"
           "the same machine. In this case use ECF_SSL=1, then\n"
           "ecflow_server expects the following files in $HOME/.ecflowrc/ssl\n\n"
           "   - dh2048.pem\n"
           "   - server.crt\n"
           "   - server.key\n"
           "   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key\n\n"
           "ecflow_client expects the following files in : $HOME/.ecflowrc/ssl\n\n"
           "   - server.crt (this must be the same as server)\n\n"
           "Alternatively you can have different setting for each server ECF_SSL=<host>.<port>\n"
           "Then server expect files of the type:\n\n"
           "   - <host>.<port>.pem\n"
           "   - <host>.<port>.crt\n"
           "   - <host>.<port>.key\n"
           "   - <host>.<port>.passwd (optional)\n\n"
           "and client expect files of the type:\n\n"
           "   - <host>.<port>.crt  # as before this must be same as the server\n\n"
           "The server/client will automatically check existence of both variants,\n"
           "but will give preference to NON <host>.<port>.*** variants first, when ECF_SSL=1\n"
           "The following steps, show you how to create the certificate files.\n"
           "This may need to be adapted if you want to use <host>.<port>.***\n\n"
           "- Generate a password protected private key. This will request a pass phrase.\n"
           "  This key is a 1024 bit RSA key which is encrypted using Triple-DES and stored\n"
           "  in a PEM format so that it is readable as ASCII text\n\n"
           "     > openssl genrsa -des3 -out server.key 1024   # Password protected private key\n"
           "- Additional security.\n"
           "  If you want additional security, create a file called 'server.passwd' and add the pass phrase to the "
           "file.\n"
           "  Then set the file permission so that file is only readable by the server process.\n"
           "  Or you can choose to remove password requirement. In that case we don't need server.passwd file.\n\n"
           "     > cp server.key server.key.secure\n"
           "     > openssl rsa -in server.key.secure -out server.key  # remove password requirement\n"
           "- Sign certificate with private key (self signed certificate).Generate Certificate Signing Request(CSR).\n"
           "  This will prompt with a number of questions.\n"
           "  However please ensure 'common name' matches the host where your server is going to run.\n\n"
           "     > openssl req -new -key server.key -out server.csr # Generate Certificate Signing Request(CSR)\n"
           "- Generate a self signed certificate CRT, by using the CSR and private key.\n\n"
           "     > openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt\n\n"
           "- Generate dhparam file. ecFlow expects 2048 key.\n"
           "     > openssl dhparam -out dh2048.pem 2048";
}

void Openssl::check_server_certificates() const {
    //   cout << "Openssl::check_server_certificates: ssl'" << ssl_ << "'\n";

    {
        string server_key = key();
        if (!fs::exists(server_key))
            throw std::runtime_error("Error: The password protected private server key file '" + server_key +
                                     "' does not exist\n\n" + ssl_info());
    }
    {
        string server_pem = pem();
        if (!fs::exists(server_pem))
            throw std::runtime_error("Error: The dhparam file(pem) '" + server_pem + "' does not exist\n\n" +
                                     ssl_info());
    }
}

std::ostream& operator<<(std::ostream& os, const ecf::Openssl& ssl) {
    ssl.print(os);
    return os;
}

} // namespace ecf
