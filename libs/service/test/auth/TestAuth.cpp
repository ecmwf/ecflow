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

#include "TestContentProvider.hpp"
#include "ecflow/core/Overload.hpp"
#include "ecflow/service/auth/Credentials.hpp"

BOOST_AUTO_TEST_SUITE(U_Auth)

BOOST_AUTO_TEST_SUITE(T_Credentials)

BOOST_AUTO_TEST_CASE(can_create_default_credentials) {
    using namespace ecf::service::auth;

    Credentials credentials;

    auto found = credentials.value("inexistent");
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(can_retrieve_existing_key) {
    using namespace ecf::service::auth;

    Credentials credentials;
    credentials.add("key", "value");

    auto found = credentials.value("key");
    BOOST_CHECK(found.has_value());
    BOOST_CHECK_EQUAL(found.value(), "value");
}

BOOST_AUTO_TEST_CASE(throw_exceptions_when_unable_to_parse_credentials) {
    using namespace ecf::service::auth;

    std::string content = R"(
        {
            INVALID JSON
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) { BOOST_FAIL("Expected error, but got credential"); },
                             [](const Credentials::Error& error) {
                                 BOOST_CHECK(error.message.find("Credentials: Unable to parse content, due to ") !=
                                             std::string::npos);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_return_error_when_invalid_content_found) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) { BOOST_FAIL("Expected error, but got credential"); },
                             [](const Credentials::Error& error) {
                                 BOOST_CHECK(error.message.find("Credentials: Invalid content found") !=
                                             std::string::npos);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_load_credentials_with_key) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "key": "00000000111111110000000011111111",
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) {
                                 auto found = credentials.key();
                                 BOOST_CHECK(found);
                                 BOOST_CHECK_EQUAL(found->key, "00000000111111110000000011111111");
                             },
                             [](const Credentials::Error& error) {
                                 BOOST_FAIL("Expected credentials, but got error: " + error.message);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_load_credentials_with_username_and_password) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "username": "user",
            "password": "pass",
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) {
                                 auto found = credentials.user();
                                 BOOST_CHECK(found);
                                 BOOST_CHECK_EQUAL(found->username, "user");
                                 BOOST_CHECK_EQUAL(found->password, "pass");
                             },
                             [](const Credentials::Error& error) {
                                 BOOST_FAIL("Expected credentials, but got error: " + error.message);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_load_credentials_with_key_and_username_and_password) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "key": "00000000111111110000000011111111",
            "username": "user",
            "password": "pass",
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) {
                                 {
                                     auto found = credentials.key();
                                     BOOST_CHECK(found);
                                     BOOST_CHECK_EQUAL(found->key, "00000000111111110000000011111111");
                                 }

                                 {
                                     auto found = credentials.user();
                                     BOOST_CHECK(found);
                                     BOOST_CHECK_EQUAL(found->username, "user");
                                     BOOST_CHECK_EQUAL(found->password, "pass");
                                 }
                             },
                             [](const Credentials::Error& error) {
                                 BOOST_FAIL("Expected credentials, but got error: " + error.message);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_credentials_with_array_type) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "key": ["00000000111111110000000011111111", "11111111000000001111111100000000"],
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) { BOOST_FAIL("Expected error, but got credentials"); },
                             [](const Credentials::Error& error) {
                                 BOOST_CHECK(error.message.find("Credentials: Unable to retrieve content, due to") !=
                                             std::string::npos);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_credentials_with_object_type) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "key": {},
            "email": "user@host.int"
        }
    )";

    auto loaded = Credentials::load_content(content);

    std::visit(ecf::overload{[](const Credentials credentials) { BOOST_FAIL("Expected error, but got credentials"); },
                             [](const Credentials::Error& error) {
                                 BOOST_CHECK(error.message.find("Credentials: Unable to retrieve content, due to") !=
                                             std::string::npos);
                             }},
               loaded);
}

BOOST_AUTO_TEST_CASE(can_load_credentials_from_file) {
    using namespace ecf::test;
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "key": "00000000111111110000000011111111",
            "username": "user",
            "password": "pass",
            "email": "user@host.int"
        }
    )";

    TestContentProvider provider("credentials", content);

    auto loaded = Credentials::load(provider.file());

    std::visit(ecf::overload{[](const Credentials credentials) {
                                 {
                                     auto found = credentials.key();
                                     BOOST_CHECK(found);
                                     BOOST_CHECK_EQUAL(found->key, "00000000111111110000000011111111");
                                 }

                                 {
                                     auto found = credentials.user();
                                     BOOST_CHECK(found);
                                     BOOST_CHECK_EQUAL(found->username, "user");
                                     BOOST_CHECK_EQUAL(found->password, "pass");
                                 }
                             },
                             [](const Credentials::Error& error) {
                                 BOOST_FAIL("Expected credentials, but got error: " + error.message);
                             }},
               loaded);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
