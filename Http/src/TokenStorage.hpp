#ifndef TOKENSTORAGE_HPP
#define TOKENSTORAGE_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : TokenStorage
// Author      : partio
// Revision    : $Revision$ 
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#ifdef ECF_OPENSSL
#include <string>
#include <vector>
#include <chrono>

struct Token {
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
          const std::chrono::system_clock::time_point& revoked_) :
       hash(hash_), salt(salt_), method(method_), description(description_), expires(expires_), revoked(revoked_)
    {}
};

class TokenStorage {
   public:
       static const TokenStorage& instance() {
           static TokenStorage instance_;
           return instance_;
       }
       TokenStorage(const TokenStorage&) = delete;
       TokenStorage(TokenStorage&&) = delete;
       TokenStorage& operator=(const TokenStorage&) = delete;
       TokenStorage& operator=(TokenStorage&&) = delete;

       bool verify(const std::string& token) const;

   private:
       TokenStorage();
       ~TokenStorage() = default;
       void ReadStorage();

       std::vector<Token> tokens_;
};

#endif
#endif
