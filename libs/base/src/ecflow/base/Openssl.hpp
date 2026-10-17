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
    ///
    /// @brief Constructs an SSL configuration with SSL disabled.
    ///
    /// The SSL context is created later by `init_for_server()` or `init_for_client()` when SSL is enabled.
    ///
    Openssl() = default;

    Openssl(const Openssl&)            = delete;
    Openssl& operator=(const Openssl&) = delete;
    Openssl(Openssl&&)                 = delete;
    Openssl& operator=(Openssl&&)      = delete;

    ///
    /// @brief Destroys the SSL configuration and its context.
    ///
    ~Openssl() = default;

    ///
    /// @brief Returns the selected certificate name.
    ///
    /// An empty string means SSL is disabled. The value is `1` for the shared certificate, or
    /// `<host>.<port>` for a server-specific certificate.
    ///
    /// @return The selected certificate name, without its directory or `.crt` suffix.
    ///
    const std::string& ssl() const { return ssl_; }

    ///
    /// @brief Checks whether SSL is enabled.
    ///
    /// @return True if a certificate has been selected; otherwise, false.
    ///
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

    ///
    /// @brief Enables SSL using shared-first certificate selection.
    ///
    /// Uses `server.crt` first, falling back to `<host>.<port>.crt` only if the shared file is absent.
    /// The ECF_SSL environment variable is ignored.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @throws std::runtime_error if neither certificate file exists; SSL is left disabled.
    /// @throws std::filesystem::filesystem_error if certificate lookup or absolute path resolution fails.
    ///
    void enable(std::string host, const std::string& port);

    ///
    /// @brief Enables SSL according to ECF_SSL when the variable is set.
    ///
    /// Certificate lookup order:
    ///
    /// - Empty or "1": `server.crt` first, then `<host>.<port>.crt` if absent.
    /// - Any other value: `<host>.<port>.crt` first, then `server.crt` if absent.
    ///
    /// If ECF_SSL is unset, the current SSL state is preserved.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @throws std::runtime_error if SSL is requested but neither certificate file exists; SSL is left disabled.
    /// @throws std::filesystem::filesystem_error if certificate lookup or absolute path resolution fails.
    ///
    void enable_if_defined(std::string host, const std::string& port);

    ///
    /// @brief Disables SSL and clears the selected certificate.
    ///
    /// The ECF_SSL environment variable is preserved. Never throws.
    ///
    void disable() { ssl_.clear(); }

    ///
    /// @brief Enables SSL preferring the server-specific certificate, independently of ECF_SSL.
    ///
    /// Uses the same certificate selection as ECF_SSL=X, falling back to server.crt only if absent.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @throws std::runtime_error if neither the server-specific nor shared certificate file exists.
    ///
    void enable_prefer_specific(std::string host, const std::string& port);

    ///
    /// @brief Creates or replaces the SSL context for server connections.
    ///
    /// Allows TLS 1.3 or later. Does nothing if SSL is disabled.
    /// Loads files from ECF_SSL_DIR, or $HOME/.ecflowrc/ssl if unset:
    ///
    /// - Shared: `server.crt`, `server.key` and `dh2048.pem`, falling back to `dh1024.pem` if absent.
    /// - Server-specific: `<host>.<port>.crt`, `<host>.<port>.key` and `<host>.<port>.pem`.
    ///
    /// Encrypted keys use the matching `.passwd` file, or the default passphrase `test` if it is absent.
    /// Loading failures do not trigger certificate fallback.
    ///
    /// @throws std::runtime_error if the private key or DH parameters are missing, or a passphrase file is unreadable.
    /// @throws boost::system::system_error if context configuration or loading certificate, key or DH files fails.
    /// @throws std::filesystem::filesystem_error if checking required files fails.
    ///
    void init_for_server();

    ///
    /// @brief Initializes the SSL context for client connections.
    ///
    /// Creates a TLS client context and loads the selected certificate as a trust anchor for server verification.
    /// Loading failures do not trigger certificate fallback.
    ///
    /// Does nothing if SSL is disabled or initialization was already attempted.
    /// Initialization is attempted at most once per instance, even if it fails.
    ///
    /// @throws boost::system::system_error if context creation or loading the selected certificate fails.
    ///
    void init_for_client();

    ///
    /// @brief Returns the initialized SSL context.
    ///
    /// The context must have been initialized by `init_for_server()` or `init_for_client()`.
    ///
    /// @return A reference to the SSL context.
    /// @pre An SSL context has been initialized.
    ///
    boost::asio::ssl::context& context();

    ///
    /// @brief Returns guidance for configuring SSL certificates.
    ///
    /// @return A null-terminated string describing certificate selection and required files.
    ///
    static const char* ssl_info();

    ///
    /// @brief Writes the selected certificate name to a stream.
    ///
    /// Writes the same value returned by `ssl()`; writes nothing when SSL is disabled.
    ///
    /// @param[out] os Stream to receive the certificate name.
    ///
    void print(std::ostream& os) const { os << ssl_; }

    ///
    /// @brief Describes the selected SSL certificate for diagnostic logging.
    ///
    /// @return The absolute certificate path and its shared or server-specific category, or "SSL disabled".
    /// @throws std::filesystem::filesystem_error if the absolute certificate path cannot be determined.
    ///
    std::string info() const;

    ///
    /// @brief Returns the selected certificate filename.
    ///
    /// @return `server.crt` for the shared certificate, `<host>.<port>.crt` for a server-specific
    /// certificate, or an empty string when SSL is disabled.
    ///
    std::string selected_crt() const;

private:
    ///
    /// @brief Checks that the selected certificate's private key and DH parameters exist.
    ///
    /// @throws std::runtime_error if either required file is missing.
    /// @throws std::filesystem::filesystem_error if checking a file fails.
    ///
    void check_server_certificates() const;

    ///
    /// @brief Loads the selected certificate into an SSL context for peer verification.
    ///
    /// @param[in,out] context SSL context to configure.
    ///
    void load_verify_file(boost::asio::ssl::context&);

    ///
    /// @brief Returns the configured certificate directory.
    ///
    /// Uses ECF_SSL_DIR when set, otherwise `$HOME/.ecflowrc/ssl`.
    ///
    /// @return Certificate directory path.
    ///
    std::string certificates_dir() const;

    ///
    /// @brief Reads the selected private key passphrase.
    ///
    /// Reads the matching `.passwd` file, removes one trailing newline, and returns `test` if the file is absent.
    ///
    /// @return Private key passphrase.
    /// @throws std::runtime_error if the passphrase file exists but cannot be read.
    ///
    std::string get_password() const;

    ///
    /// @brief Returns the selected DH parameter file path.
    ///
    /// For the shared certificate, uses `dh2048.pem` if present, otherwise `dh1024.pem`.
    /// For a server-specific certificate, uses `<host>.<port>.pem`.
    ///
    /// @return DH parameter file path.
    ///
    std::string pem() const;

    ///
    /// @brief Returns the selected certificate file path.
    ///
    /// @return `server.crt` for the shared certificate, otherwise `<host>.<port>.crt`.
    ///
    std::string crt() const;

    ///
    /// @brief Returns the selected private key file path.
    ///
    /// @return `server.key` for the shared certificate, otherwise `<host>.<port>.key`.
    ///
    std::string key() const;

    ///
    /// @brief Returns the selected private key passphrase file path.
    ///
    /// @return `server.passwd` for the shared certificate, otherwise `<host>.<port>.passwd`.
    ///
    std::string passwd() const;

private:
    ///
    /// Selected certificate name: empty when disabled, `1` for shared, or `<host>.<port>` for server-specific.
    ///
    std::string ssl_;

    ///
    /// SSL context, created on demand when SSL is enabled.
    ///
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;

    ///
    /// Records whether client context initialization has been attempted.
    ///
    bool init_for_client_{false};
};

///
/// @brief Writes the selected certificate name to a stream.
///
/// @param[out] os Stream to receive the certificate name.
/// @param[in] ssl SSL configuration to write.
/// @return The output stream.
///
std::ostream& operator<<(std::ostream& os, const ecf::Openssl&);

} // namespace ecf
