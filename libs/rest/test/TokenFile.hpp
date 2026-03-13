/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_test_TokenFile_HPP
#define ecflow_http_test_TokenFile_HPP

#include <array>
#include <iomanip>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "ecflow/test/scaffold/Naming.hpp"

///
/// @brief A token entry to be written into the token file.
///
struct BearerToken
{
    /// @brief The token (plaintext, 64-char hex string)
    std::string token;

    /// @brief A description of the token
    std::string description;

    /// @brief expiration time, in ISO 8601 format (if empty, means no expiry)
    std::string expires_at;

    /// @brief revocation time, in ISO 8601 format (if empty, means no revocation)
    std::string revoked_at;
};

class TokenFile {
public:
    TokenFile() = delete;

    ///
    /// @brief Create a token file from a list of tokens.
    ///
    /// Each provided token is hashed (HMAC-SHA256 with a random salt)
    /// and written to expected JSON file format.
    ///
    /// @param tokenfile Path to the token file to create.
    /// @param tokens   Tokens to write.
    ///
    TokenFile(const std::string& tokenfile, const std::vector<BearerToken>& tokens);

    ~TokenFile();

    ///
    /// @brief Generate a random token as a 64-character lowercase hex string (256 bits of entropy).
    ///
    /// @param num_bytes The number of random bytes (default: 32, producing 64 hex chars).
    /// @return The generated token string.
    ///
    static std::string generate_token(std::size_t num_bytes = 32);

private:
    ///
    /// @brief Generate a random salt as a 16-character lowercase hex string.
    ///
    /// @return The generated salt string.
    ///
    static std::string generate_salt();

    ///
    /// @briref Computes HMAC-SHA256 of the provided token using the provided salt, returning the hex-encoded digest.
    ///
    /// @return The generated HMAC-SHA256
    ///
    static std::string hmac_sha256(const std::string& salt, const std::string& token);

    std::string tokenfile_;
};

inline TokenFile::TokenFile(const std::string& tokenfile, const std::vector<BearerToken>& tokens)
    : tokenfile_(tokenfile) {

    nlohmann::json j = nlohmann::json::array();

    for (const auto& token : tokens) {
        const std::string salt       = generate_salt();
        const std::string hashed     = hmac_sha256(salt, token.token);
        const std::string hash_value = "sha256$" + salt + "$" + hashed;

        nlohmann::json token_obj = {
            {"hash", hash_value},
            {"description", token.description},
        };

        if (!token.expires_at.empty()) {
            token_obj["expires_at"] = token.expires_at;
        }
        if (!token.revoked_at.empty()) {
            token_obj["revoked_at"] = token.revoked_at;
        }

        j.push_back(token_obj);
    }

    std::ofstream o(tokenfile_);
    o << std::setw(4) << j << std::endl;
    setenv("ECF_API_TOKEN_FILE", tokenfile_.c_str(), 1);
}

inline TokenFile::~TokenFile() {
    if (remove(tokenfile_.c_str()) != 0) {
        ECF_TEST_ERR(<< "Failed to remove token file " << tokenfile_);
    }
    else {
        BOOST_TEST_MESSAGE("Removed token file " << tokenfile_);
    }
}

inline std::string TokenFile::generate_token(std::size_t num_bytes) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    std::ostringstream buffer;
    buffer << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < num_bytes; ++i) {
        buffer << std::setw(2) << dist(gen);
    }

    return buffer.str();
}

inline std::string TokenFile::generate_salt() {
    return generate_token(16);
}

inline std::string TokenFile::hmac_sha256(const std::string& salt, const std::string& token) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
    unsigned int hash_len;

    HMAC(EVP_sha256(),
         salt.c_str(),
         static_cast<int>(salt.size()),
         reinterpret_cast<const unsigned char*>(token.c_str()),
         token.size(),
         hash.data(),
         &hash_len);

    std::ostringstream buffer;
    buffer << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; ++i) {
        buffer << std::setw(2) << static_cast<int>(hash[i]);
    }

    return buffer.str();
}

#endif /* #ifndef ecflow_http_test_TokenFile_HPP */
