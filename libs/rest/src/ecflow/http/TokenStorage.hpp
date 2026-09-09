/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_http_TokenStorage_HPP
#define ecflow_http_TokenStorage_HPP

#include <chrono>
#include <string>
#include <vector>

namespace ecf::http {

struct Token
{
    std::string hash;
    std::string salt;
    std::string method;
    std::string description;
    std::chrono::system_clock::time_point expires;
    std::chrono::system_clock::time_point revoked;

    Token(const std::string& hash_,
          const std::string& salt_,
          const std::string& method_,
          const std::string& description_,
          const std::chrono::system_clock::time_point& expires_,
          const std::chrono::system_clock::time_point& revoked_)
        : hash(hash_),
          salt(salt_),
          method(method_),
          description(description_),
          expires(expires_),
          revoked(revoked_) {}
};

class TokenStorage {
private:
    TokenStorage();

public:
    TokenStorage(const TokenStorage&)            = delete;
    TokenStorage(TokenStorage&&)                 = delete;
    TokenStorage& operator=(const TokenStorage&) = delete;
    TokenStorage& operator=(TokenStorage&&)      = delete;

    ~TokenStorage() = default;

    static const TokenStorage& instance() {
        static TokenStorage instance_;
        return instance_;
    }

    bool verify(const std::string& token) const;

private:
    void ReadStorage();

    std::vector<Token> tokens_;
};

} // namespace ecf::http

#endif /* ecflow_http_TokenStorage_HPP */
