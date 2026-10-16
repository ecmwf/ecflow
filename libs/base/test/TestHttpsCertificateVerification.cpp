// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "ecflow/base/ConnectionDiagnosis.hpp"
#include "ecflow/base/HttpClient.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

///
/// \brief Tests that the HTTPS client verifies the certificate of the server before sending a request
///
/// An HTTPS peer is stood in on the loopback interface, presenting a self-signed certificate for
/// `localhost`. The peer refuses every request with 403, so that a request reaching it is told apart
/// from one stopped by the verification of the certificate.
///

#if defined(CPPHTTPLIB_OPENSSL_SUPPORT)

    #include <openssl/evp.h>
    #include <openssl/pem.h>
    #include <openssl/rsa.h>
    #include <openssl/x509.h>

using ecf::ConnectionDiagnosis;
using ecf::ConnectionFailure;
using ecf::test::scaffold::MakeDirectory;
using ecf::test::scaffold::WithTestEnvironmentVariable;

namespace {

///
/// @brief Owns a private key and a self-signed certificate, generated in memory.
///
class SelfSignedCertificate {
public:
    ///
    /// @brief Generates a 2048-bit RSA key and a certificate for the given common name, valid for one day.
    ///
    /// @param[in] common_name The host name the certificate is issued for
    /// @throws std::runtime_error if OpenSSL fails to generate the key or the certificate
    ///
    explicit SelfSignedCertificate(const std::string& common_name) {
        EVP_PKEY_CTX* context = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        bool generated        = context && EVP_PKEY_keygen_init(context) == 1 &&
                         EVP_PKEY_CTX_set_rsa_keygen_bits(context, 2048) == 1 && EVP_PKEY_keygen(context, &key_) == 1;
        EVP_PKEY_CTX_free(context);
        if (!generated) {
            throw std::runtime_error("Unable to generate the RSA key of the test certificate");
        }

        certificate_ = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(certificate_), 1);
        X509_gmtime_adj(X509_getm_notBefore(certificate_), 0);
        X509_gmtime_adj(X509_getm_notAfter(certificate_), 24L * 60 * 60);
        X509_set_pubkey(certificate_, key_);

        X509_NAME* name = X509_NAME_new();
        X509_NAME_add_entry_by_txt(
            name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(common_name.c_str()), -1, -1, 0);
        X509_set_subject_name(certificate_, name);
        X509_set_issuer_name(certificate_, name);
        X509_NAME_free(name);

        if (X509_sign(certificate_, key_, EVP_sha256()) == 0) {
            throw std::runtime_error("Unable to sign the test certificate");
        }
    }

    SelfSignedCertificate(const SelfSignedCertificate&)            = delete;
    SelfSignedCertificate& operator=(const SelfSignedCertificate&) = delete;
    SelfSignedCertificate(SelfSignedCertificate&&)                 = delete;
    SelfSignedCertificate& operator=(SelfSignedCertificate&&)      = delete;

    ~SelfSignedCertificate() {
        X509_free(certificate_);
        EVP_PKEY_free(key_);
    }

    X509* certificate() const { return certificate_; }
    EVP_PKEY* key() const { return key_; }

    ///
    /// @brief Writes the certificate, in PEM format, to the given file.
    ///
    /// @param[in] path The file to write; replaced if it exists
    /// @throws std::runtime_error if the file cannot be written
    ///
    void write_certificate(const std::string& path) const {
        FILE* file = std::fopen(path.c_str(), "wb");
        if (!file) {
            throw std::runtime_error("Unable to open " + path + " for writing");
        }
        bool written = PEM_write_X509(file, certificate_) == 1;
        std::fclose(file);
        if (!written) {
            throw std::runtime_error("Unable to write the test certificate to " + path);
        }
    }

private:
    EVP_PKEY* key_     = nullptr;
    X509* certificate_ = nullptr;
};

///
/// @brief An HTTPS peer on the loopback interface, refusing every request with 403.
///
/// The peer binds to a port chosen by the system, so that concurrent tests never contend for a fixed
/// port. It serves connections until it is destroyed.
///
class HttpsPeer {
public:
    explicit HttpsPeer(const SelfSignedCertificate& certificate)
        : server_(certificate.certificate(), certificate.key()) {
        if (!server_.is_valid()) {
            throw std::runtime_error("Unable to set up the TLS context of the test peer");
        }
        server_.Post("/v1/ecflow", [](const httplib::Request&, httplib::Response& response) { response.status = 403; });
        // Binding also listens, so connections made before the worker accepts them wait in the backlog
        port_ = server_.bind_to_any_port("127.0.0.1");
        if (port_ <= 0) {
            throw std::runtime_error("Unable to bind the test peer");
        }
        worker_ = std::thread([this]() { server_.listen_after_bind(); });
    }

    HttpsPeer(const HttpsPeer&)            = delete;
    HttpsPeer& operator=(const HttpsPeer&) = delete;
    HttpsPeer(HttpsPeer&&)                 = delete;
    HttpsPeer& operator=(HttpsPeer&&)      = delete;

    ~HttpsPeer() {
        server_.stop();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    std::string port() const { return std::to_string(port_); }

private:
    httplib::SSLServer server_;
    int port_ = 0;
    std::thread worker_;
};

/// Runs an HTTPS request against the given endpoint, and returns the resulting diagnosis.
ConnectionDiagnosis run_https_request(const std::string& host, const std::string& port) {
    ConnectionDiagnosis diagnosis;
    HttpClient client(std::make_shared<CtsCmd>(CtsCmd::PING), "https", host, port, 5, &diagnosis);
    client.run();
    return diagnosis;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_HttpsCertificateVerification)

BOOST_AUTO_TEST_CASE(test_https_client_rejects_an_untrusted_certificate) {
    ECF_NAME_THIS_TEST();

    SelfSignedCertificate certificate("localhost");
    HttpsPeer peer(certificate);

    auto diagnosis = run_https_request("localhost", peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::CertificateRejected);
}

BOOST_AUTO_TEST_CASE(test_https_client_accepts_a_trusted_certificate) {
    ECF_NAME_THIS_TEST();

    SelfSignedCertificate certificate("localhost");
    HttpsPeer peer(certificate);

    auto directory = MakeDirectory().create();
    auto trusted   = (directory.path() / "trusted.crt").string();
    certificate.write_certificate(trusted);
    WithTestEnvironmentVariable trust_store("SSL_CERT_FILE", trusted);

    auto diagnosis = run_https_request("localhost", peer.port());

    // The request reached the peer, which refused it
    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::RejectedRequest);
}

BOOST_AUTO_TEST_CASE(test_https_client_rejects_a_trusted_certificate_for_another_host) {
    ECF_NAME_THIS_TEST();

    SelfSignedCertificate certificate("localhost");
    HttpsPeer peer(certificate);

    auto directory = MakeDirectory().create();
    auto trusted   = (directory.path() / "trusted.crt").string();
    certificate.write_certificate(trusted);
    WithTestEnvironmentVariable trust_store("SSL_CERT_FILE", trusted);

    // The certificate names localhost, not the address used to reach the peer
    auto diagnosis = run_https_request("127.0.0.1", peer.port());

    BOOST_CHECK_EQUAL(diagnosis.failure, ConnectionFailure::CertificateRejected);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#endif
