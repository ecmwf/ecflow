// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include <boost/asio/ssl.hpp>

namespace ecf {

class Openssl {
    static constexpr const char* shared_envvar_value_ = "1";

public:
    ///
    /// @brief Represents a request to enable SSL and its certificate lookup order.
    ///
    /// An unset ECF_SSL variable leaves SSL disabled; an explicitly empty value requests shared-first selection.
    /// The environment value is retained only when the request originated from ECF_SSL.
    ///
    class Option {
    public:
        ///
        /// @brief Identifies the requested SSL certificate lookup order.
        ///
        enum class Preference { Disabled, SharedFirst, SpecificFirst };

        ///
        /// @brief Constructs a disabled SSL request.
        ///
        Option() = default;

        ///
        /// @brief Requests SSL with shared-first selection, independently of ECF_SSL.
        ///
        /// @return A shared-first option.
        ///
        static Option make_option_shared() { return Option(Preference::SharedFirst); }

        ///
        /// @brief Requests SSL with specific-first selection, independently of ECF_SSL.
        ///
        /// @return A specific-first option.
        ///
        static Option make_option_specific() { return Option(Preference::SpecificFirst); }

        ///
        /// @brief Parses a supplied ECF_SSL value without reading the environment.
        ///
        /// @param[in] value Empty or "1" selects shared-first; any other value selects specific-first.
        /// @return An enabled option retaining the supplied value, including an explicitly empty value.
        ///
        static Option make_from_envvar_value(std::string value);

        ///
        /// @brief Reads ECF_SSL, distinguishing an unset variable from an explicitly empty value.
        ///
        /// @return A parsed option, or a disabled option when ECF_SSL is unset.
        ///
        static Option make_from_envvar();

        ///
        /// @brief Reports whether SSL was requested.
        ///
        /// @return True for either enabled preference.
        ///
        [[nodiscard]] bool enabled() const { return preference_ != Preference::Disabled; }

        ///
        /// @brief Reports whether shared-first selection was requested.
        ///
        /// @return True for the shared-first preference.
        ///
        [[nodiscard]] bool shared() const { return preference_ == Preference::SharedFirst; }

        ///
        /// @brief Describes the requested lookup order.
        ///
        /// @return The shared-first or specific-first strategy, or an empty string when SSL was not requested.
        ///
        [[nodiscard]] const char* strategy() const;

        ///
        /// @brief Describes how SSL was requested and the resulting lookup order, for debug output.
        ///
        /// @param[in] via_option True when the `--ssl` command-line option was given.
        /// @return The source of the request (`--ssl` and/or ECF_SSL, with its value) followed by the strategy.
        ///
        [[nodiscard]] std::string describe(bool via_option) const;

        ///
        /// @brief Returns the supplied ECF_SSL value without interpreting it as a file path.
        ///
        /// @return The original value, including an empty value, or no value for an explicit request.
        ///
        [[nodiscard]] const std::optional<std::string>& envvar_value() const { return envvar_value_; }

    private:
        ///
        /// @brief Constructs a requested mode and its optional source environment value.
        ///
        /// @param[in] preference Requested lookup order, or Disabled.
        /// @param[in] value Original ECF_SSL value if supplied; an empty string is distinct from no value.
        ///
        explicit Option(Preference preference, std::optional<std::string> value = std::nullopt)
            : preference_(preference),
              envvar_value_(std::move(value)) {}

        ///
        /// Requested mode, independent of which certificate is ultimately selected.
        ///
        Preference preference_{Preference::Disabled};

        ///
        /// Original ECF_SSL value, including an explicitly empty string; absent for CLI and UI requests.
        ///
        std::optional<std::string> envvar_value_;
    };

    ///
    /// @brief Identifies a selected SSL certificate and its certificate directory.
    ///
    /// The directory is captured at selection time. File paths are derived from it and the selected stem;
    /// the shared certificate uses the legacy `1` value only when exported as ECF_SSL.
    ///
    class Certificate {
    public:
        ///
        /// @brief Creates the shared certificate selection.
        ///
        /// @param[in] directory Absolute certificate directory captured during selection.
        /// @return A certificate identifying `server.crt`.
        ///
        static Certificate make_shared_certificate(std::filesystem::path directory);

        ///
        /// @brief Creates a server-specific certificate selection.
        ///
        /// @param[in] directory Absolute certificate directory captured during selection.
        /// @param[in] stem Selected `<host>.<port>` filename stem.
        /// @return A certificate identifying the server-specific files.
        ///
        static Certificate make_specific_certificate(std::filesystem::path directory, std::string stem);

        ///
        /// @brief Reports whether the selected certificate is shared.
        ///
        /// @return True for `server.crt`.
        ///
        [[nodiscard]] bool shared() const { return stem_ == shared_stem_; }

        ///
        /// @brief Returns the effective ECF_SSL value for the selected certificate.
        ///
        /// @return `1` for shared, otherwise the selected `<host>.<port>` stem.
        ///
        [[nodiscard]] std::string to_envvar_value() const { return shared() ? shared_envvar_value_ : stem_; }

        ///
        /// @brief Returns the full path to the selected certificate.
        ///
        /// @return The shared or server-specific `.crt` path.
        ///
        [[nodiscard]] std::filesystem::path crt_file() const { return directory_ / (stem_ + ".crt"); }

        ///
        /// @brief Returns the full path to the selected private key.
        ///
        /// @return The shared or server-specific `.key` path.
        ///
        [[nodiscard]] std::filesystem::path key_file() const { return directory_ / (stem_ + ".key"); }

        ///
        /// @brief Returns the full path to the selected passphrase file.
        ///
        /// @return The shared or server-specific `.passwd` path.
        ///
        [[nodiscard]] std::filesystem::path passwd_file() const { return directory_ / (stem_ + ".passwd"); }

        ///
        /// @brief Returns the full path to the selected DH parameters.
        ///
        /// For shared certificates, prefers `dh2048.pem` and falls back to `dh1024.pem` when absent.
        ///
        /// @return The shared or server-specific `.pem` path.
        /// @throws std::filesystem::filesystem_error if checking the shared DH file fails.
        ///
        [[nodiscard]] std::filesystem::path pem_file() const;

    private:
        static constexpr const char* shared_stem_ = "server";

        ///
        /// @brief Stores the selected stem and certificate directory.
        ///
        /// @param[in] directory Absolute directory captured during certificate lookup.
        /// @param[in] stem `server` for a shared certificate, or `<host>.<port>` for a specific certificate.
        ///
        Certificate(std::filesystem::path directory, std::string stem);

        ///
        /// Directory captured during selection, so later environment changes cannot change the loaded files.
        ///
        std::filesystem::path directory_;

        ///
        /// Selected filename stem: `server` for shared, otherwise `<host>.<port>`.
        ///
        std::string stem_;
    };

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
    /// @brief Returns the requested SSL mode and its optional environment value.
    ///
    /// @return The request, including its lookup order when SSL was requested.
    ///
    const Option& requested() const { return requested_; }

    ///
    /// @brief Returns the selected certificate after lookup and fallback.
    ///
    /// @return The selected certificate, or no certificate when SSL is disabled or lookup failed.
    ///
    const std::optional<Certificate>& certificate() const { return selected_; }

    ///
    /// @brief Checks whether SSL is enabled.
    ///
    /// @return True if a certificate has been selected; otherwise, false.
    ///
    bool enabled() const { return selected_.has_value(); }

    ///
    /// @brief Tries to enable SSL using an explicit option, without reading ECF_SSL.
    ///
    /// The certificate directory is still read from ECF_SSL_DIR, or derived from HOME.
    /// Returns false when neither certificate exists; filesystem errors can still throw.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @param[in] option Requested certificate preference.
    /// @return True if either certificate exists; false if neither exists, leaving SSL disabled.
    /// @throws std::filesystem::filesystem_error if a certificate lookup fails.
    ///
    bool try_enable_from_option(std::string host, const std::string& port, Option option);

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
    void enable_prefer_shared(std::string host, const std::string& port);

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
    void enable_based_on_envvar(std::string host, const std::string& port);

    ///
    /// @brief Disables SSL and clears the selected certificate.
    ///
    /// The ECF_SSL environment variable is preserved. Never throws.
    ///
    void disable() {
        requested_ = Option{};
        selected_.reset();
    }

    ///
    /// @brief Enables SSL preferring the server-specific certificate, independently of ECF_SSL.
    ///
    /// Uses the same certificate selection as ECF_SSL=X, falling back to server.crt only if absent.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @throws std::runtime_error if neither the server-specific nor shared certificate file exists.
    /// @throws std::filesystem::filesystem_error if certificate lookup or absolute path resolution fails.
    ///
    void enable_prefer_specific(std::string host, const std::string& port);

    ///
    /// @brief Tries to enable SSL for a connection to another server, following the selection of this server.
    ///
    /// A shared selection (`1`) of this server requests shared-first selection for the peer; a server-specific
    /// selection requests specific-first selection. The ECF_SSL environment variable is ignored.
    ///
    /// @param[in] host Peer server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Peer server port used to select the certificate.
    /// @param[in] server_ssl Effective ECF_SSL value of this server, as returned by `AbstractServer::ssl()`.
    /// @return True if either certificate of the peer exists; false if neither exists, leaving SSL disabled.
    /// @throws std::filesystem::filesystem_error if a certificate lookup fails.
    ///
    bool try_enable_for_peer(std::string host, const std::string& port, const std::string& server_ssl);

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
    /// Writes the environment-compatible selected certificate; writes nothing when SSL is disabled.
    ///
    /// @param[out] os Stream to receive the certificate name.
    ///
    void print(std::ostream& os) const {
        if (selected_) {
            os << selected_->to_envvar_value();
        }
    }

    ///
    /// @brief Describes the selected SSL certificate for diagnostic logging.
    ///
    /// @return The absolute certificate path and its shared or server-specific category, or "SSL disabled".
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
    /// @brief Enables SSL using an explicit option, reporting a missing certificate as an error.
    ///
    /// @param[in] host Server hostname; localhost is resolved to the local hostname.
    /// @param[in] port Server port used to select the certificate.
    /// @param[in] option Requested certificate preference; must be enabled.
    /// @throws std::runtime_error if neither certificate file exists; SSL is left disabled.
    /// @throws std::filesystem::filesystem_error if certificate lookup or absolute path resolution fails.
    ///
    void enable_or_throw(std::string host, const std::string& port, const Option& option);

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
    /// Requested SSL mode, disabled by default; preserved on lookup failure for diagnostics.
    ///
    Option requested_;

    ///
    /// Selected certificate, absent until a certificate has been found.
    ///
    std::optional<Certificate> selected_;

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
