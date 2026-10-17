// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/Openssl.hpp"

#include <cassert>
#include <stdexcept>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

namespace {

///
/// @brief Formats a missing-certificate error in the requested lookup order.
///
/// @param[in] directory Configured certificate directory, which may be relative.
/// @param[in] host Server hostname after localhost resolution.
/// @param[in] port Server port used in the specific certificate filename.
/// @param[in] option Requested lookup order, which determines the order of the paths and the strategy.
/// @return An error containing both absolute certificate paths and the requested strategy.
/// @throws std::filesystem::filesystem_error if an absolute path cannot be determined.
///
std::string missing_certificate_error(const std::string& directory,
                                      const std::string& host,
                                      const std::string& port,
                                      const Openssl::Option& option) {
    const auto shared   = fs::absolute(fs::path{directory} / "server.crt").lexically_normal().string();
    const auto specific = fs::absolute(fs::path{directory} / (host + "." + port + ".crt")).lexically_normal().string();
    return "Error: Unable to find certificate file: '" + (option.shared() ? shared : specific) + "' or '" +
           (option.shared() ? specific : shared) + "' (using strategy: " + option.strategy() + ")";
}

///
/// @brief Resolves `localhost` to the local hostname, as used in server-specific certificate names.
///
/// @param[in] host Server hostname.
/// @return The local hostname for `localhost`, otherwise @p host unchanged.
///
std::string resolve_host(std::string host) {
    if (host == ecf::string_constants::localhost) {
        return Host().name();
    }
    return host;
}

} // namespace

const char* Openssl::Option::strategy() const {
    switch (preference_) {
        case Preference::SharedFirst:
            return "shared certificate first, server-specific certificate as fallback";
        case Preference::SpecificFirst:
            return "server-specific certificate first, shared certificate as fallback";
        case Preference::Disabled:
            break;
    }
    return "";
}

std::string Openssl::Option::describe(bool via_option) const {
    std::string source;
    if (envvar_value_) {
        source = std::string(via_option ? "via --ssl with " : "via ") + "ECF_SSL='" + *envvar_value_ + "'";
    }
    else {
        source = "via --ssl, ECF_SSL unset";
    }
    return source + " (" + strategy() + ")";
}

Openssl::Option Openssl::Option::make_from_envvar_value(std::string value) {
    const auto preference =
        value.empty() || value == shared_envvar_value_ ? Preference::SharedFirst : Preference::SpecificFirst;
    return Option(preference, std::move(value));
}

Openssl::Option Openssl::Option::make_from_envvar() {
    if (auto value = ecf::environment::fetch<std::string>(ecf::environment::ECF_SSL); value) {
        return make_from_envvar_value(value.value());
    }
    return {};
}

Openssl::Certificate Openssl::Certificate::make_shared_certificate(fs::path directory) {
    return Certificate(std::move(directory), shared_stem_);
}

Openssl::Certificate Openssl::Certificate::make_specific_certificate(fs::path directory, std::string stem) {
    return Certificate(std::move(directory), std::move(stem));
}

Openssl::Certificate::Certificate(fs::path directory, std::string stem)
    : directory_(std::move(directory)),
      stem_(std::move(stem)) {
}

fs::path Openssl::Certificate::pem_file() const {
    if (shared()) {
        const auto preferred = directory_ / "dh2048.pem";
        return fs::exists(preferred) ? preferred : directory_ / "dh1024.pem";
    }
    return directory_ / (stem_ + ".pem");
}

std::string Openssl::info() const {
    if (!enabled()) {
        return "SSL disabled";
    }

    const auto path = selected_->crt_file().string();
    return "SSL certificate selected: '" + path + "' (" + (selected_->shared() ? "shared" : "server-specific") + ")";
}

std::string Openssl::selected_crt() const {
    if (enabled()) {
        return selected_->crt_file().filename().string();
    }

    return "";
}

bool Openssl::try_enable_from_option(std::string host, const std::string& port, Option option) {
    requested_ = std::move(option);
    selected_.reset();
    reset_context();
    if (!requested_.enabled()) {
        return false;
    }
    host = resolve_host(std::move(host));

    const fs::path directory = fs::absolute(certificates_dir()).lexically_normal();
    Certificate shared       = Certificate::make_shared_certificate(directory);
    Certificate specific     = Certificate::make_specific_certificate(directory, host + "." + port);
    const auto& first        = requested_.shared() ? shared : specific;
    const auto& second       = requested_.shared() ? specific : shared;
    if (fs::exists(first.crt_file())) {
        selected_ = first;
        return true;
    }
    if (fs::exists(second.crt_file())) {
        selected_ = second;
        return true;
    }
    return false;
}

void Openssl::enable_or_throw(std::string host, const std::string& port, const Option& option) {
    host = resolve_host(std::move(host));
    if (!try_enable_from_option(host, port, option)) {
        throw std::runtime_error(missing_certificate_error(certificates_dir(), host, port, option));
    }
}

void Openssl::enable_prefer_shared(std::string host, const std::string& port) {
    enable_or_throw(std::move(host), port, Option::make_option_shared());
}

void Openssl::enable_based_on_envvar(std::string host, const std::string& port) {
    if (auto option = Option::make_from_envvar(); option.enabled()) {
        enable_or_throw(std::move(host), port, option);
    }
}

void Openssl::enable_prefer_specific(std::string host, const std::string& port) {
    enable_or_throw(std::move(host), port, Option::make_option_specific());
}

bool Openssl::try_enable_for_peer(std::string host, const std::string& port, const std::string& server_ssl) {
    return try_enable_from_option(std::move(host), port, Option::make_from_envvar_value(server_ssl));
}

boost::asio::ssl::context& Openssl::context() {
    assert(ssl_context_);
    return *ssl_context_;
}

void Openssl::init_for_server() {
    if (enabled()) {
        check_server_certificates();

        // --- Server-side SSL context initialisation ---
        //
        // The context is created with `tls_server`, the generic server-side
        // TLS method.  On its own this permits any TLS version that the
        // underlying OpenSSL library supports.  The `set_options` call below
        // then explicitly disables every protocol version below TLS 1.3
        // (SSLv2, SSLv3, TLS 1.0, TLS 1.1, TLS 1.2), leaving TLS 1.3 as
        // the *only* version the server will accept.
        //
        // The practical consequence is that any use of weak cryptography is eliminated:
        //   - SSLv2/SSLv3 are broken (POODLE, DROWN) -- disabled.
        //   - TLS 1.0/1.1 rely on SHA-1 and RC4 that are no longer acceptable -- disabled.
        //   - TLS 1.2 cipher suites can include DES/3DES, RC4 and other weak
        //     algorithms that are no longer acceptable -- disabled.
        //
        //   - TLS 1.3 mandates AEAD cipher suites (AES-GCM, ChaCha20-Poly1305)
        //     and forward secrecy for every connection.
        //      ** TLS 1.3 is the effective (and only) option after set_options. **
        //
        // Notice that using `tls_server` enables the automatic use of future TLS versions, simply by recompiling
        // against a newer OpenSSL library that supports them. The server will automatically negotiate the highest
        // mutually supported TLS version with the client, always guaranteeing the TLS 1.3 is the lowest accepted in
        // the handshake with the client. This is a future-proof design that does not require code changes to support
        // new TLS versions as they are released, while still enforcing the use of strong cryptography.
        // Even so, changes will be need if TLS 1.3 becomes deprecated.
        //
        // Client compliance requirement:
        //   During the TLS handshake the client proposes a list of protocol
        //   versions and cipher suites it supports.  Because the server
        //   advertises *only* TLS 1.3, any client that does not support
        //   TLS 1.3 will fail the handshake with a "protocol version" alert.
        //   The counterpart `init_for_client()` therefore uses the generic
        //   `tls_client` method (which supports TLS 1.3) so that the
        //   negotiation succeeds. Clients built against older OpenSSL versions
        //   that lack TLS 1.3 support will be rejected.
        //
        // Other options:
        //   - `default_workarounds` enables OpenSSL bug-compatibility shims
        //     that are harmless on TLS 1.3 but kept for robustness.
        //   - `single_dh_use` ensures a fresh DH key pair is generated for
        //     each handshake, preserving forward secrecy for the DH key
        //     exchange used by the loaded dh2048.pem parameters.
        //
        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tls_server);

        // clang-format off
        ssl_context_->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::no_tlsv1_2 |
            boost::asio::ssl::context::single_dh_use);
        // clang-format on

        // The password callback is registered before any private key is loaded.
        // OpenSSL calls it immediately during `use_private_key_file` if the key file is passphrase-protected.
        ssl_context_->set_password_callback(
            [this](std::size_t size, boost::asio::ssl::context_base::password_purpose purpose) {
                return this->get_password();
            });
        ssl_context_->use_certificate_chain_file(selected_->crt_file().string());
        ssl_context_->use_private_key_file(selected_->key_file().string(), boost::asio::ssl::context::pem);
        ssl_context_->use_tmp_dh_file(selected_->pem_file().string());
    }
}

void Openssl::init_for_client() {
    if (!init_for_client_ && enabled()) {
        init_for_client_ = true;

        // --- Client-side SSL context initialisation ---
        //
        // `tls_client` is the generic, version-flexible client method: it
        // allows OpenSSL to negotiate the highest mutually supported TLS
        // version.  Because the server is configured exclusively for TLS 1.3
        // (see `init_for_server`), the handshake will always settle on
        // TLS 1.3; the flexibility here is on the client side only and does
        // not weaken security.
        //
        // Certificate verification:
        //   `load_verify_file` loads the self-signed server certificate (CRT)
        //   as a trusted CA.  During the handshake OpenSSL verifies the
        //   server's presented certificate against this file, ensuring the
        //   client is talking to the expected server and not an impostor
        //   (i.e. protection against man-in-the-middle attacks).
        //
        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tls_client);
        ssl_context_->load_verify_file(selected_->crt_file().string());
    }
}

std::string Openssl::get_password() const {
    std::string passwd_file = selected_->passwd_file().string();
    if (fs::exists(passwd_file)) {
        std::string contents;
        if (ecf::File::open(passwd_file, contents)) {
            // remove /n added by editor.
            if (!contents.empty() && contents[contents.size() - 1] == '\n') {
                contents.erase(contents.begin() + contents.size() - 1);
            }
            return contents;
        }
        else {
            throw std::runtime_error(
                MESSAGE("Error: Unable to open password file '" << passwd_file << "' (" << strerror(errno) << ")"));
        }
    }
    return "test";
}

std::string Openssl::certificates_dir() const {
    if (auto found = ecf::environment::fetch<std::string>("ECF_SSL_DIR"); found) {
        return found.value();
    }
    else {
        return ecf::environment::get<std::string>("HOME") + "/.ecflowrc/ssl/";
    }
}

const char* Openssl::ssl_info() {
    return "ecFlow client and server are SSL enabled. To use SSL choose between:\n"
           "  1. export ECF_SSL=X    # recommended: search <host>.<port>.crt, then server.crt\n"
           "  2. export ECF_SSL=1    # shared certificate: search server.crt, then <host>.<port>.crt\n"
           "  3. use --ssl           # use ECF_SSL if set; otherwise, use shared-first mode\n"
           "                         # Typically ssl server can be started with ecflow_start.sh -s\n"
           "  4. Client.enable_ssl() # for python client\n\n"
           "ecflow_ui uses the specific-first strategy (ECF_SSL=X) and ignores ECF_SSL.\n"
           "ecFlow expects certificates in $HOME/.ecflowrc/ssl unless ECF_SSL_DIR overrides the directory.\n"
           "Shared certificates should be avoided. Use ECF_SSL=1 only for intentionally shared certificates;\n"
           "ECF_SSL=1 keeps its shared-first selection for compatibility. In this mode,\n"
           "ecflow_server expects the following files in the certificate directory:\n\n"
           "   - dh2048.pem\n"
           "   - server.crt\n"
           "   - server.key\n"
           "   - server.passwd (optional) if this exists it must contain the pass phrase used to create server.key\n\n"
           "ecflow_client expects the following file in the certificate directory:\n\n"
           "   - server.crt (this must be the same as server)\n\n"
           "The recommended ECF_SSL=X mode uses separate certificates for each server.\n"
           "In this mode, the server expects:\n\n"
           "   - <host>.<port>.pem\n"
           "   - <host>.<port>.crt\n"
           "   - <host>.<port>.key\n"
           "   - <host>.<port>.passwd (optional)\n\n"
           "The client requires:\n\n"
           "   - <host>.<port>.crt  # as before this must be same as the server\n\n"
           "The following steps create the certificate files for the shared mode.\n"
           "For server-specific certificates, replace 'server' and 'dh2048' with <host>.<port>.\n\n"
           "- Generate a password protected private key. This will request a pass phrase.\n"
           "  This key is a 1024 bit RSA key which is encrypted using Triple-DES and stored\n"
           "  in a PEM format so that it is readable as ASCII text\n\n"
           "     > openssl genrsa -des3 -out server.key 1024   # Password protected private key\n"
           "- Additional security.\n"
           "  For additional security, create a file called 'server.passwd' and add the pass phrase to the "
           "file.\n"
           "  Then set the file permission so that file is only readable by the server process.\n"
           "  Alternatively, remove the password requirement; server.passwd is then not needed.\n\n"
           "     > cp server.key server.key.secure\n"
           "     > openssl rsa -in server.key.secure -out server.key  # remove password requirement\n"
           "- Sign certificate with private key (self-signed certificate). Generate Certificate Signing Request(CSR).\n"
           "  This will prompt with a number of questions.\n"
           "  However please ensure 'common name' matches the host where your server is going to run.\n\n"
           "     > openssl req -new -key server.key -out server.csr # Generate Certificate Signing Request(CSR)\n"
           "- Generate a self-signed certificate CRT, by using the CSR and private key.\n\n"
           "     > openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt\n\n"
           "- Generate dhparam file. ecFlow expects 2048 key.\n"
           "     > openssl dhparam -out dh2048.pem 2048";
}

void Openssl::check_server_certificates() const {
    {
        std::string server_key = selected_->key_file().string();
        if (!fs::exists(server_key)) {
            throw std::runtime_error("Error: The password protected private server key file '" + server_key +
                                     "' does not exist\n\n" + ssl_info());
        }
    }
    {
        std::string server_pem = selected_->pem_file().string();
        if (!fs::exists(server_pem)) {
            throw std::runtime_error("Error: The dhparam file(pem) '" + server_pem + "' does not exist\n\n" +
                                     ssl_info());
        }
    }
}

std::ostream& operator<<(std::ostream& os, const ecf::Openssl& ssl) {
    ssl.print(os);
    return os;
}

} // namespace ecf
