/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Base64.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Base64)

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_enconde) {
    ECF_NAME_THIS_TEST();

    std::string input    = "Hello, World!";
    std::string expected = "SGVsbG8sIFdvcmxkIQ==";
    BOOST_CHECK_EQUAL(ecf::encode_base64(input), expected);
}

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_decode) {
    ECF_NAME_THIS_TEST();

    std::string input    = "SGVsbG8sIFdvcmxkIQ==";
    std::string expected = "Hello, World!";
    BOOST_CHECK_EQUAL(ecf::decode_base64(input), expected);
}

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_decode_and_encode) {
    ECF_NAME_THIS_TEST();

    std::string input   = "Hello, World!";
    std::string encoded = ecf::encode_base64(input);
    std::string decoded = ecf::decode_base64(encoded);
    BOOST_CHECK_EQUAL(decoded, input);
}

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_decode_and_encode_empty_string) {
    ECF_NAME_THIS_TEST();

    std::string input   = "";
    std::string encoded = ecf::encode_base64(input);
    std::string decoded = ecf::decode_base64(encoded);
    BOOST_CHECK_EQUAL(decoded, input);
}

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_decode_and_encode_long_string) {
    ECF_NAME_THIS_TEST();

    std::string input =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut "
        "labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco "
        "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
        "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat "
        "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
    std::string encoded = ecf::encode_base64(input);
    std::string decoded = ecf::decode_base64(encoded);
    BOOST_CHECK_EQUAL(decoded, input);
}
BOOST_AUTO_TEST_CASE(test_base64_is_able_to_decode_and_encode_long_string_with_newlines) {
    ECF_NAME_THIS_TEST();

    std::string input =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut "
        "labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco "
        "laboris nisi ut aliquip ex ea commodo consequat.\nDuis aute irure dolor in reprehenderit in "
        "voluptate velit esse cillum dolore eu fugiat nulla pariatur.\nExcepteur sint occaecat cupidatat "
        "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
    std::string encoded = ecf::encode_base64(input);
    std::string decoded = ecf::decode_base64(encoded);
    BOOST_CHECK_EQUAL(decoded, input);
}

BOOST_AUTO_TEST_CASE(test_base64_is_able_to_validate) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::validate_base64("SGVsbG8sIFdvcmxkIQ=="));
    BOOST_CHECK(ecf::validate_base64("U29tZSB0ZXh0IHdpdGggcGFkZGluZw=="));
    BOOST_CHECK(!ecf::validate_base64("Invalid base64 string!"));
    BOOST_CHECK(!ecf::validate_base64("SGVsbG8sIFdvcmxkIQ")); // missing padding
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
