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

#include "ecflow/core/Identity.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Identity;
using ecf::Password;
using ecf::Username;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Identity)

BOOST_AUTO_TEST_CASE(test_username_value) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Username{"alice"}.value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_username_equality) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Username{"alice"} == Username{"alice"});
}

BOOST_AUTO_TEST_CASE(test_username_inequality) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Username{"alice"} != Username{"bob"});
}

BOOST_AUTO_TEST_CASE(test_password_value) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Password{"secret"}.value(), "secret");
}

BOOST_AUTO_TEST_CASE(test_password_equality) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Password{"secret"} == Password{"secret"});
}

BOOST_AUTO_TEST_CASE(test_password_inequality) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Password{"secret"} != Password{"other"});
}

BOOST_AUTO_TEST_CASE(test_identity_none_is_neither_user_nor_task) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_none();
    BOOST_CHECK(!identity.is_user());
    BOOST_CHECK(!identity.is_task());
}

BOOST_AUTO_TEST_CASE(test_identity_none_has_empty_username) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_none().username().value(), "");
}

BOOST_AUTO_TEST_CASE(test_identity_none_has_empty_password) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_none().password().value(), "");
}

BOOST_AUTO_TEST_CASE(test_identity_user_preserves_username) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_user("alice", "secret").username().value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_identity_user_preserves_password) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_user("alice", "secret").password().value(), "secret");
}

BOOST_AUTO_TEST_CASE(test_identity_user_is_user) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_user("alice", "secret");
    BOOST_CHECK(identity.is_user());
    BOOST_CHECK(!identity.is_task());
}

BOOST_AUTO_TEST_CASE(test_identity_user_is_neither_custom_nor_secure) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_user("alice", "secret");
    BOOST_CHECK(!identity.is_custom());
    BOOST_CHECK(!identity.is_secure());
}

BOOST_AUTO_TEST_CASE(test_identity_custom_user_preserves_password) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_custom_user("alice", "secret").password().value(), "secret");
}

BOOST_AUTO_TEST_CASE(test_identity_custom_user_is_user) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Identity::make_custom_user("alice", "secret").is_user());
}

BOOST_AUTO_TEST_CASE(test_identity_custom_user_is_custom) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_custom_user("alice", "secret");
    BOOST_CHECK(identity.is_custom());
    BOOST_CHECK(!identity.is_secure());
}

BOOST_AUTO_TEST_CASE(test_identity_secure_user_preserves_username) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_secure_user("alice").username().value(), "alice");
}

BOOST_AUTO_TEST_CASE(test_identity_secure_user_suppresses_password) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_secure_user("alice").password().value(), "");
}

BOOST_AUTO_TEST_CASE(test_identity_secure_user_is_user) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK(Identity::make_secure_user("alice").is_user());
}

BOOST_AUTO_TEST_CASE(test_identity_secure_user_is_secure) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_secure_user("alice");
    BOOST_CHECK(identity.is_secure());
    BOOST_CHECK(!identity.is_custom());
}

BOOST_AUTO_TEST_CASE(test_identity_task_is_task) {
    ECF_NAME_THIS_TEST();
    const auto identity = Identity::make_task("pid", "pass", "1");
    BOOST_CHECK(identity.is_task());
    BOOST_CHECK(!identity.is_user());
}

BOOST_AUTO_TEST_CASE(test_identity_task_maps_pid_to_username) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_task("pid", "pass", "1").username().value(), "pid");
}

BOOST_AUTO_TEST_CASE(test_identity_task_maps_pass_to_password) {
    ECF_NAME_THIS_TEST();
    BOOST_CHECK_EQUAL(Identity::make_task("pid", "pass", "1").password().value(), "pass");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
