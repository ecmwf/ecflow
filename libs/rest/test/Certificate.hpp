/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

///
/// Based on https://gist.github.com/nathan-osman/5041136
///

#ifndef ecflow_http_test_Certificate_HPP
#define ecflow_http_test_Certificate_HPP

#include <cstdio>
#include <iostream>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "ecflow/test/scaffold/Naming.hpp"

class Certificate {
public:
    Certificate() = delete;
    explicit Certificate(const std::string& path);
    ~Certificate();

private:
    std::string path_;
    EVP_PKEY* generate_key();
    X509* generate_x509(EVP_PKEY* pkey);
    void write_to_disk(EVP_PKEY* pkey, X509* x509);
};

inline Certificate::Certificate(const std::string& path) : path_(path) {
    // Generating RSA key...
    EVP_PKEY* pkey = generate_key();

    // Generating x509 certificate...
    X509* x509 = generate_x509(pkey);
    if (!x509) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Unable to create certificate");
        ;
    }

    // Writing key and certificate to disk...
    write_to_disk(pkey, x509);
}

inline Certificate::~Certificate() {
    if (remove((path_ + "/server.crt").c_str()) != 0) {
        ECF_TEST_ERR(<< "Failed to remove file server.crt");
    }
    else {
        ECF_TEST_ERR(<< "Removed file " << (path_ + "/server.crt"));
    }

    if (remove((path_ + "/server.key").c_str()) != 0) {
        ECF_TEST_ERR(<< "Failed to remove file server.key");
    }
    else {
        ECF_TEST_ERR(<< "Removed file " << (path_ + "/server.key"));
    }
}

/* Generates a 2048-bit RSA key. */
inline EVP_PKEY* Certificate::generate_key() {
    const int KEY_LEN = 2048;
#if OPENSSL_VERSION_MAJOR >= 3
    EVP_PKEY* pkey = EVP_RSA_gen(KEY_LEN);
    if (!pkey) {
        throw std::runtime_error("Unable to create EVP_PKEY structure");
    }
#else
    /* Allocate memory for the EVP_PKEY structure. */
    EVP_PKEY* pkey = EVP_PKEY_new();

    /* Generate the RSA key and assign it to pkey. */
    RSA* rsa    = RSA_new();
    BIGNUM* bne = BN_new();

    if (!BN_set_word(bne, 65537)) {
        throw std::runtime_error("Unable to set BIGNUM");
    }

    if (!RSA_generate_key_ex(rsa, KEY_LEN, bne, NULL)) {
        throw std::runtime_error("Unable to generate 2048-bit RSA key");
    }

    // RSA * rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Unable to generate 2048-bit RSA key");
    }
#endif

    /* The key has been generated, return it. */
    return pkey;
}

/* Generates a self-signed x509 certificate. */
inline X509* Certificate::generate_x509(EVP_PKEY* pkey) {
    /* Allocate memory for the X509 structure. */
    X509* x509 = X509_new();
    if (!x509) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Unable to create X509 structure");
    }

    /* Set the serial number. */
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    /* This certificate is valid from now until exactly one year from now. */
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);

    /* Set the public key for our certificate. */
    X509_set_pubkey(x509, pkey);

    /* We want to copy the subject name to the issuer name. */
    X509_NAME* name = X509_get_subject_name(x509);

    /* Set the country code and common name. */
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"CA", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"ecflow-test", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"localhost", -1, -1, 0);

    /* Now set the issuer name. */
    X509_set_issuer_name(x509, name);

    /* Actually sign the certificate with our key. */
    if (!X509_sign(x509, pkey, EVP_sha1())) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Error signing certificate");
    }

    return x509;
}

inline void Certificate::write_to_disk(EVP_PKEY* pkey, X509* x509) {
    /* Open the PEM file for writing the key to disk. */

    FILE* pkey_file = fopen((path_ + "/server.key").c_str(), "wb");
    if (!pkey_file) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Unable to open " + path_ + "/server.key for writing");
    }

    /* Write the key to disk. */
    bool ret = PEM_write_PrivateKey(pkey_file, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(pkey_file);

    EVP_PKEY_free(pkey);

    if (!ret) {
        X509_free(x509);
        throw std::runtime_error("Unable to write private key to disk");
    }

    /* Open the PEM file for writing the certificate to disk. */
    FILE* x509_file = fopen((path_ + "/server.crt").c_str(), "wb");
    if (!x509_file) {
        X509_free(x509);
        throw std::runtime_error("Unable to open " + path_ + "/server.crt for writing");
    }

    /* Write the certificate to disk. */
    ret = PEM_write_X509(x509_file, x509);
    fclose(x509_file);

    X509_free(x509);
    if (!ret) {
        throw std::runtime_error("Unable to write private key to disk");
    }
}

#endif /* ecflow_http_test_Certificate_HPP */
