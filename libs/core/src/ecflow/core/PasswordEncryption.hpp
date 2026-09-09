/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_PasswordEncryption_HPP
#define ecflow_core_PasswordEncryption_HPP

///
/// \brief Provides a generic password encryption mechanism + specific POSIX implementation
///

#include <stdexcept>
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

#endif /* ecflow_core_PasswordEncryption_HPP */
