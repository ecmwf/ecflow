#ifndef PASSWORD_ENCRYPTION_HPP_
#define PASSWORD_ENCRYPTION_HPP_
//============================================================================
// Name        :
// Author      :
// Revision    :
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : Provides a generic password encryption mechanism
//                + specific POSIX implementation
//============================================================================

#include <string>

#include <unistd.h>

struct PosixEncryption
{
    using salt_t      = std::string;
    using key_t       = std::string;
    using encrypted_t = std::string;

    static encrypted_t encrypt(const key_t& key, const salt_t& salt) {
        auto result = crypt(key.c_str(), salt.c_str());
        if (!result) {
            throw std::runtime_error("Error: unable to encrypt the given key");
        }
        return std::string{result};
    }
};

template <typename ENGINE>
struct BasePasswordEncryption
{
    using username_t           = std::string;
    using plain_password_t     = std::string;
    using encrypted_password_t = std::string;

    static encrypted_password_t encrypt(const plain_password_t& password, const username_t& username) {
        return ENGINE::encrypt(password, username);
    }
};

using PasswordEncryption = BasePasswordEncryption<PosixEncryption>;

#endif // PASSWORD_ENCRYPTION_HPP_
