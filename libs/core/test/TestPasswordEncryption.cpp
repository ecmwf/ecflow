/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

///
/// \brief Tests the functionality provided by PasswordEncryption
///

#include <boost/test/unit_test.hpp>

#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_PasswordEncryption)

BOOST_AUTO_TEST_CASE(test_is_able_to_encrypt_password) {
    ECF_NAME_THIS_TEST();

    PasswordEncryption::username_t user               = "username";
    PasswordEncryption::plain_password_t plain        = "password";
    PasswordEncryption::encrypted_password_t expected = "usjRS48E8ZADM";
    // Expected was obtained by calling `$ openssl passwd -salt username password`

    auto encrypted = PasswordEncryption::encrypt(plain, user);

    BOOST_REQUIRE(encrypted == expected);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
