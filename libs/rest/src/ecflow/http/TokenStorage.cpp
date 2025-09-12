/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/TokenStorage.hpp"

#include <atomic> // shared mutex only with c++14
#include <fstream>
#include <iomanip>
#include <shared_mutex>
#include <sstream>
#include <thread>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/JSON.hpp"
#include "ecflow/http/Options.hpp"

namespace ecf::http {

std::shared_mutex m;

std::atomic initialized = false;

namespace {

std::string hmac_sha256(const std::string& salt, const std::string& token) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    HMAC(EVP_sha256(),
         salt.c_str(),
         static_cast<int>(salt.size()),
         reinterpret_cast<const unsigned char*>(token.c_str()),
         token.size(),
         hash,
         &hash_len);
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string hmac_pbkdf2_sha256(const std::string& salt, const std::string& token, int num_iterations) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    PKCS5_PBKDF2_HMAC(token.c_str(),
                      static_cast<int>(token.size()),
                      reinterpret_cast<const unsigned char*>(salt.c_str()),
                      static_cast<int>(salt.size()),
                      num_iterations,
                      EVP_sha256(),
                      SHA256_DIGEST_LENGTH,
                      hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string hash(const std::string& method, const std::string& salt, const std::string& token) {
    /*
     * Hash a token with correct HMAC.
     * Current options are:
     * - sha256
     * - pbkdf2:sha256
     */

    if (method == "sha256") {
        return hmac_sha256(salt, token);
    }
    else if (method.find("pbkdf2:sha256") != std::string::npos) {
        int num_iterations = 260000;

        try {
            num_iterations = std::stoi(method.substr(method.find(":", 8) + 1, std::string::npos));
        }
        catch (const std::exception& e) {
            printf("ERROR: method %s has invalid number of iterations: %s\n", method.c_str(), e.what());
        }
        return hmac_pbkdf2_sha256(salt, token, num_iterations);
    }

    throw HttpServerException(HttpStatusCode::server_error_internal_server_error, "Unsupported HMAC: " + method);
}

bool is_supported_hmac(std::string_view method) {
    if (method == "sha256" || method.find("pbkdf2:sha256") != std::string::npos) {
        return true;
    }
    return false;
}

std::chrono::system_clock::time_point time_point_from_isostring(const std::string& str) {
    if (str.empty()) {
        return std::chrono::system_clock::time_point{};
    }

    std::tm tm = {};
    strptime(str.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace

TokenStorage::TokenStorage() {
    std::thread t(&TokenStorage::ReadStorage, this);
    t.detach();

    while (initialized.load() == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool TokenStorage::verify(const std::string& token) const {
    const auto now = std::chrono::system_clock::now();

    // Take a (weak) reader lock; the background thread updating
    // tokens from file can block us but other readers cannot

    std::shared_lock lock(m);
    for (const auto& t : tokens_) {
        const std::string hashed = hash(t.method, t.salt, token);
        if (hashed == t.hash && (t.expires.time_since_epoch().count() == 0 || t.expires > now) &&
            (t.revoked.time_since_epoch().count() == 0 || t.revoked > now)) {
            if (opts.verbose) {
                printf("Token for '%s' authenticated successfully\n", t.description.c_str());
            }
            return true;
        }
    }
    return false;
}

std::vector<Token> ReadTokens(const std::string& filename) {
    std::ifstream ifs(filename);
    auto j = ojson::parse(ifs);
    std::vector<Token> new_tokens;

    /* Read a token from json file.
     * Format is:
     * [
     * {
     *    "hash" : "...",        # hashed and salted token, format is identical to python
                                 # library 'werkzeug': METHOD$SALT$HASH
          "description" : "...", # free-form description of token (application)
          "expires_at" : NUM,    # unix epoch time when this token will expire
                                 # OPTIONAL: if missing or zero, no expiration time is set
          "revoked_at" : NUM     # unix epoch time when this token was revoked
                                 # OPTIONAL: if missing or zero, no revoke time is set
     * }
     * ]
     */

    for (const auto& o : j) {
        try {
            std::chrono::system_clock::time_point expires_at; // optional
            std::chrono::system_clock::time_point revoked_at; // optional

            if (o.contains("expires_at")) {
                expires_at = time_point_from_isostring(o.at("expires_at").get<std::string>());
            }

            if (o.contains("revoked_at")) {
                revoked_at = time_point_from_isostring(o.at("revoked_at").get<std::string>());
            }

            const std::string& hash = o.at("hash").get<std::string>();

            std::vector<std::string> elems;
            ecf::Str::split(hash, elems, "$");

            if (elems.size() != 3) {
                throw std::invalid_argument("Invalid hash format: " + hash);
            }
            if (is_supported_hmac(elems[0]) == false) {
                throw std::invalid_argument("Unsupported HMAC: " + elems[0]);
            }

            new_tokens.emplace_back(
                elems[2], elems[1], elems[0], o.at("description").get<std::string>(), expires_at, revoked_at);
        }
        catch (const std::exception& e) {
            printf("Invalid token: %s: %s\n", o.dump().c_str(), e.what());
        }
    }
    if (opts.verbose) {
        printf("Read %ld tokens\n", new_tokens.size());
    }

    return new_tokens;
}

void TokenStorage::ReadStorage() {
    const std::chrono::seconds sleep_time(20);
    std::chrono::system_clock::time_point last_modified;

    // Scan periodically the api-token file and read the
    // contents automatically if they are updated

    while (true) {
        try {
            auto current_modified = ecf::fsx::last_write_time(fs::path(opts.tokens_file));
            if (current_modified > last_modified) {
                auto new_tokens = ReadTokens(opts.tokens_file);
                {
                    std::lock_guard lock(m);
                    tokens_ = new_tokens;
                }
                last_modified = current_modified;
            }
        }
        catch (const fs::filesystem_error& e) {
            printf("ERROR: API token file (%s) read failed: %s\n", opts.tokens_file.c_str(), e.what());
        }
        catch (const ojson::exception& e) {
            printf("ERROR: API token file (%s) parse failed: %s\n", opts.tokens_file.c_str(), e.what());
        }
        initialized = true;
        std::this_thread::sleep_for(sleep_time);
    }
}

} // namespace ecf::http
