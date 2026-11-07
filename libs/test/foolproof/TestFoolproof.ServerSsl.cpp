// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

#ifdef ECF_OPENSSL

    #include <openssl/evp.h>
    #include <openssl/pem.h>
    #include <openssl/rsa.h>
    #include <openssl/x509.h>

namespace {

// The 2048-bit finite field Diffie-Hellman group ffdhe2048 (RFC 7919), a public constant
constexpr const char* dh_parameters = "-----BEGIN DH PARAMETERS-----\n"
                                      "MIIBCAKCAQEA//////////+t+FRYortKmq/cViAnPTzx2LnFg84tNpWp4TZBFGQz\n"
                                      "+8yTnc4kmz75fS/jY2MMddj2gbICrsRhetPfHtXV/WVhJDP1H18GbtCFY2VVPe0a\n"
                                      "87VXE15/V8k1mE8McODmi3fipona8+/och3xWKE2rec1MKzKT0g6eXq8CrGCsyT7\n"
                                      "YdEIqUuyyOP7uWrat2DX9GgdT0Kj3jlN9K5W7edjcrsZCwenyO4KbXCeAvzhzffi\n"
                                      "7MA0BM0oNC9hkXL+nOmFg/+OTxIy7vKBg8P+OxtMb61zO7X8vC7CIAXFjvGDfRaD\n"
                                      "ssbzSibBsu/6iGtCOGEoXJf//////////wIBAg==\n"
                                      "-----END DH PARAMETERS-----\n";

///
/// @brief Writes a new self-signed certificate, its unprotected private key and the DH parameters.
///
/// Each call generates a new key, so that certificates written by different calls never verify each other.
///
/// @param[in] directory Existing directory owned by the test
/// @param[in] stem Base name of the files: `<stem>.crt` and `<stem>.key`
/// @param[in] dh Name of the DH parameters file
/// @throws std::runtime_error if a key, the certificate or a file cannot be created.
///
void write_certificate(const fs::path& directory, const std::string& stem, const std::string& dh) {
    auto check = [](bool success, const std::string& what) {
        if (!success) {
            throw std::runtime_error("Unable to " + what);
        }
    };

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr),
                                                                        &EVP_PKEY_CTX_free);
    check(context && EVP_PKEY_keygen_init(context.get()) > 0 &&
              EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), 2048) > 0,
          "set up the RSA key generation");
    EVP_PKEY* raw_key = nullptr;
    check(EVP_PKEY_keygen(context.get(), &raw_key) > 0, "generate an RSA key");
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(raw_key, &EVP_PKEY_free);

    std::unique_ptr<X509, decltype(&X509_free)> certificate(X509_new(), &X509_free);
    check(certificate != nullptr, "create a certificate");
    ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), 1);
    X509_gmtime_adj(X509_get_notBefore(certificate.get()), 0);
    X509_gmtime_adj(X509_get_notAfter(certificate.get()), 24 * 60 * 60);
    X509_set_pubkey(certificate.get(), key.get());
    // A separate name, since OpenSSL 4.0 returns the certificate's own names as const
    std::unique_ptr<X509_NAME, decltype(&X509_NAME_free)> name(X509_NAME_new(), &X509_NAME_free);
    check(name != nullptr, "create a certificate name");
    X509_NAME_add_entry_by_txt(
        name.get(), "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(stem.c_str()), -1, -1, 0);
    X509_set_subject_name(certificate.get(), name.get());
    X509_set_issuer_name(certificate.get(), name.get());
    check(X509_sign(certificate.get(), key.get(), EVP_sha256()) > 0, "sign the certificate");

    auto write = [&check](const fs::path& path, auto&& writer) {
        std::unique_ptr<BIO, decltype(&BIO_free)> file(BIO_new_file(path.c_str(), "w"), &BIO_free);
        check(file && writer(file.get()), "write " + path.string());
    };
    write(directory / (stem + ".key"), [&key](BIO* file) {
        return PEM_write_bio_PrivateKey(file, key.get(), nullptr, nullptr, 0, nullptr, nullptr) > 0;
    });
    write(directory / (stem + ".crt"),
          [&certificate](BIO* file) { return PEM_write_bio_X509(file, certificate.get()) > 0; });
    write(directory / dh, [](BIO* file) { return BIO_puts(file, dh_parameters) > 0; });
}

///
/// @brief Describes one client connecting to the server under test.
///
struct ClientCase
{
    std::optional<std::string> ecf_ssl; ///< Value of ECF_SSL for the client; empty optional when unset
    bool ssl_option;                    ///< Whether the client is given --ssl
    bool connects;                      ///< Whether the TLS handshake is expected to succeed
};

///
/// @brief Starts a server with --ssl, with both certificates present, and connects each client to it.
///
/// The shared (server.crt) and server-specific (<host>.<port>.crt) certificates are distinct, so a client
/// connects only when it selects the same certificate as the server.
///
/// @param[in] server_ecf_ssl Value of ECF_SSL for the server; empty optional when unset
/// @param[in] server_selects_shared Whether the server is expected to select the shared certificate, which the
///            ECF_SSL server variable records as "1", instead of the server-specific one, recorded as <host>.<port>
/// @param[in] clients Clients to connect, with the expected outcome of each
///
void check_ssl_connections(const std::optional<std::string>& server_ecf_ssl,
                           bool server_selects_shared,
                           const std::vector<ClientCase>& clients) {
    using namespace ecf::test::scaffold;

    auto startup    = MakeDirectory{}.create();
    const auto home = startup.path() / "server home";
    const auto ssl  = startup.path() / "ssl";
    fs::create_directories(home);
    fs::create_directories(ssl);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host           = MakeHost{}.create();
    auto port           = ecf::test::foolproof::reserve_port();
    const auto specific = host.value() + "." + std::to_string(port.value());

    const auto exported = server_selects_shared ? std::string("1") : specific;

    write_certificate(ssl, "server", "dh2048.pem");
    write_certificate(ssl, specific, specific + ".pem");

    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", ssl.string());
    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()},
                               "ECF_HOME = " + home.string() + "\nECF_LOG = server.log\nECF_CHECKMODE = CHECK_NEVER\n");

    std::optional<WithTestEnvironmentVariable> server_ssl;
    if (server_ecf_ssl) {
        server_ssl.emplace("ECF_SSL", *server_ecf_ssl);
    }
    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(port.value()), "--ssl"},
                    startup.path());
    server_ssl.reset();
    BOOST_SCOPE_EXIT_ALL(&) {
        process.terminate();
        process.wait();
    };

    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "SSL server did not start: " << error.message());

    for (const auto& [ecf_ssl, ssl_option, connects] : clients) {
        BOOST_TEST_CONTEXT("Client ECF_SSL: " << ecf_ssl.value_or("(unset)") << ", --ssl: " << ssl_option) {
            std::optional<WithTestEnvironmentVariable> client_ssl;
            if (ecf_ssl) {
                client_ssl.emplace("ECF_SSL", *ecf_ssl);
            }

            ClientInvoker client(host.value(), std::to_string(port.value()));
            ecf::test::foolproof::configure_client(client, host.value(), port.value());

            std::vector<std::string> args = {"ClientInvoker", "--ping"};
            if (ssl_option) {
                args.emplace_back("--ssl");
            }

            if (connects) {
                BOOST_CHECK_MESSAGE(client.invoke(args) == 0, client.errorMsg());
                BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
                BOOST_REQUIRE(client.defs());
                BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_SSL"), exported);
            }
            else {
                BOOST_CHECK_NE(client.invoke(args), 0);
                BOOST_CHECK_MESSAGE(client.errorMsg().find("TLS handshake failed while verifying the server "
                                                           "certificate") != std::string::npos,
                                    client.errorMsg());
            }
        }
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerSsl)

///
/// @brief Verifies that --ssl alone selects the server-specific certificate on both server and client.
///
BOOST_AUTO_TEST_CASE(test_server_ssl_option_alone_uses_the_specific_certificate) {
    ECF_NAME_THIS_TEST();

    check_ssl_connections(
        std::nullopt, false, {{std::nullopt, true, true}, {"X", false, true}, {"1", true, false}, {"1", false, false}});
}

///
/// @brief Verifies that ECF_SSL=1 with --ssl selects the shared certificate on both server and client.
///
BOOST_AUTO_TEST_CASE(test_server_ssl_option_with_ecf_ssl_1_uses_the_shared_certificate) {
    ECF_NAME_THIS_TEST();

    check_ssl_connections(
        "1", true, {{"1", false, true}, {"1", true, true}, {std::nullopt, true, false}, {"X", false, false}});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#endif
