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

    BOOST_CHECK_EXCEPTION(Credentials::load_content(content), std::runtime_error, [](const std::runtime_error& e) {
        return std::string{e.what()}.find("Credentials: Unable to parse content, due to ") != std::string::npos;
    });
}

BOOST_AUTO_TEST_CASE(throw_exceptions_when_invalid_content_found) {
    using namespace ecf::service::auth;

    // Doesn't provide neither 'key' nor 'username'+'password'
    std::string content = R"(
        {
            "url": "http://address:1234",
            "email": "user@host.int"
        }
    )";

    BOOST_CHECK_EXCEPTION(Credentials::load_content(content), std::runtime_error, [](const std::runtime_error& e) {
        return std::string{e.what()}.find("Credentials: Invalid content found") != std::string::npos;
    });
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

    auto credentials = Credentials::load_content(content);

    {
        auto found = credentials.key();
        BOOST_CHECK(found);
        BOOST_CHECK_EQUAL(found->key, "00000000111111110000000011111111");
    }
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

    auto credentials = Credentials::load_content(content);

    {
        auto found = credentials.user();
        BOOST_CHECK(found);
        BOOST_CHECK_EQUAL(found->username, "user");
        BOOST_CHECK_EQUAL(found->password, "pass");
    }
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

    auto credentials = Credentials::load_content(content);

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

    BOOST_CHECK_EXCEPTION(Credentials::load_content(content), std::runtime_error, [](const std::runtime_error& e) {
        return std::string{e.what()}.find("Credentials: Unable to retrieve content, due to") != std::string::npos;
    });
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

    BOOST_CHECK_EXCEPTION(Credentials::load_content(content), std::runtime_error, [](const std::runtime_error& e) {
        return std::string{e.what()}.find("Credentials: Unable to retrieve content, due to") != std::string::npos;
    });
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

    auto credentials = Credentials::load(provider.file());

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
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
