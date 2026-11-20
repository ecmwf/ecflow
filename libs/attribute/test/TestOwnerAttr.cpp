// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace ecf;
using namespace ecf::test::scaffold;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_OwnerAttr)

BOOST_AUTO_TEST_CASE(test_owner_attr_user_only) {
    ECF_NAME_THIS_TEST();

    auto owner = OwnerAttr::create("alice");

    BOOST_CHECK_EQUAL(owner.user(), "alice");
    BOOST_CHECK(!owner.group().has_value());
    BOOST_CHECK_EQUAL(owner.value(), "alice");
    BOOST_CHECK_EQUAL(owner.toString(), "owner alice");
}

BOOST_AUTO_TEST_CASE(test_owner_attr_user_and_group) {
    ECF_NAME_THIS_TEST();

    auto owner = OwnerAttr::create("alice:ops");

    BOOST_CHECK_EQUAL(owner.user(), "alice");
    BOOST_REQUIRE(owner.group().has_value());
    BOOST_CHECK_EQUAL(owner.group().value(), "ops");
    BOOST_CHECK_EQUAL(owner.value(), "alice:ops");
    BOOST_CHECK_EQUAL(owner.toString(), "owner alice:ops");
}

BOOST_AUTO_TEST_CASE(test_owner_attr_valid_names) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"a",
                                    "alice2",
                                    "svc_ecflow",
                                    "svc-ecflow",
                                    "first.last",
                                    "_svc",
                                    "a2:b2",
                                    "user:group_name",
                                    "abcdefghijklmnopqrstuvwxyz012345" /* 32 characters */}) {
        BOOST_CHECK_MESSAGE(OwnerAttr::is_valid(value), "Expected '" << value << "' to be valid");
        BOOST_CHECK_NO_THROW(OwnerAttr::create(value));
        BOOST_CHECK_EQUAL(OwnerAttr::create(value).value(), value);
    }
}

BOOST_AUTO_TEST_CASE(test_owner_attr_invalid_names) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"",
                                    ":ops",
                                    "alice:",
                                    "alice:ops:extra",
                                    "al ice",
                                    " alice",
                                    "alice\t",
                                    "-alice",
                                    "alice:-ops",
                                    "1000",
                                    "alice:1000",
                                    "alice/ops",
                                    "alice$",
                                    "ali*ce",
                                    "abcdefghijklmnopqrstuvwxyz0123456" /* 33 characters */}) {
        BOOST_CHECK_MESSAGE(!OwnerAttr::is_valid(value), "Expected '" << value << "' to be invalid");
        try {
            (void)OwnerAttr::create(value);
            BOOST_ERROR("Expected an exception for '" << value << "'");
        }
        catch (const std::runtime_error& e) {
            const std::string msg = e.what();
            BOOST_CHECK_MESSAGE(msg.find("'" + value + "'") != std::string::npos,
                                "Expected the message to name the value '" << value << "', got: " << msg);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_owner_attr_equality) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(OwnerAttr::create("alice") == OwnerAttr::create("alice"));
    BOOST_CHECK(OwnerAttr::create("alice:ops") == OwnerAttr::create("alice:ops"));
    BOOST_CHECK(!(OwnerAttr::create("alice") == OwnerAttr::create("bob")));
    BOOST_CHECK(!(OwnerAttr::create("alice") == OwnerAttr::create("alice:ops")));
    BOOST_CHECK(!(OwnerAttr::create("alice:ops") == OwnerAttr::create("alice:dev")));
}

BOOST_AUTO_TEST_CASE(test_owner_attr_serialisation) {
    ECF_NAME_THIS_TEST();

    doSaveAndRestore(AutomaticFileLocation{std::string{"owner_attr"}}.path().string(), OwnerAttr::create("alice"));
    doSaveAndRestore(AutomaticFileLocation{std::string{"owner_attr"}}.path().string(), OwnerAttr::create("alice:ops"));
}

BOOST_AUTO_TEST_CASE(test_owner_attr_restore_into_existing_owner) {
    ECF_NAME_THIS_TEST();

    std::string archive;
    ecf::save_as_string(archive, OwnerAttr::create("bob"));

    // Restoring an owner without group must not keep the group of the previous value
    auto restored = OwnerAttr::create("alice:ops");
    ecf::restore_from_string(archive, restored);

    BOOST_CHECK_EQUAL(restored.value(), "bob");
    BOOST_CHECK(!restored.group().has_value());
}

BOOST_AUTO_TEST_CASE(test_owner_attr_restore_rejects_invalid_archive) {
    ECF_NAME_THIS_TEST();

    std::string valid_user_archive;
    ecf::save_as_string(valid_user_archive, OwnerAttr::create("alice"));
    std::string valid_group_archive;
    ecf::save_as_string(valid_group_archive, OwnerAttr::create("alice:ops"));

    auto replace = [](std::string text, const std::string& from, const std::string& to) {
        auto pos = text.find(from);
        BOOST_REQUIRE(pos != std::string::npos);
        return text.replace(pos, from.size(), to);
    };

    for (const auto& archive : {replace(valid_user_archive, "\"alice\"", "\"1000\""),
                                replace(valid_user_archive, "\"alice\"", "\"-alice\""),
                                replace(valid_user_archive, "\"alice\"", "\"\""),
                                replace(valid_group_archive, "\"ops\"", "\"1000\""),
                                replace(valid_group_archive, "\"ops\"", "\"o ps\"")}) {
        OwnerAttr restored;
        BOOST_CHECK_THROW(ecf::restore_from_string(archive, restored), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_owner_attr_write_appends) {
    ECF_NAME_THIS_TEST();

    std::string text = "task t\n  ";
    OwnerAttr::create("alice:ops").write(text);

    BOOST_CHECK_EQUAL(text, "task t\n  owner alice:ops");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
