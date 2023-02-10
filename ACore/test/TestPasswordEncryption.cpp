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
// Description : Tests the functionality provided by PasswordEncryption
//============================================================================

#include <boost/test/unit_test.hpp>

#include "PasswordEncryption.hpp"

BOOST_AUTO_TEST_SUITE(CoreTestSuite)

BOOST_AUTO_TEST_CASE(test_is_able_to_encrypt_password) {
    PasswordEncryption::username_t user               = "username";
    PasswordEncryption::plain_password_t plain        = "password";
    PasswordEncryption::encrypted_password_t expected = "usjRS48E8ZADM";
    // Expected was obtained by calling `$ openssl passwd -salt username password`

    auto encrypted = PasswordEncryption::encrypt(plain, user);

    BOOST_REQUIRE(encrypted == expected);
}

BOOST_AUTO_TEST_SUITE_END()
