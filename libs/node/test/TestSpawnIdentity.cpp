// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <pwd.h>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/SpawnIdentity.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

///
/// The account a command is spawned as is resolved in the server, before the fork; the resolution
/// refuses whatever must never become a job identity.
///

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SpawnIdentity)

BOOST_AUTO_TEST_CASE(test_an_empty_owner_is_refused) {
    ECF_NAME_THIS_TEST();

    std::string error;
    BOOST_CHECK(!resolve_spawn_identity("", error));
    BOOST_CHECK_MESSAGE(error.find("no owner") != std::string::npos, "Unexpected reason: " << error);
}

BOOST_AUTO_TEST_CASE(test_an_unknown_user_is_refused) {
    ECF_NAME_THIS_TEST();

    std::string error;
    BOOST_CHECK(!resolve_spawn_identity("no_such_user_ecflow_test", error));
    BOOST_CHECK_MESSAGE(error.find("no_such_user_ecflow_test") != std::string::npos &&
                            error.find("unknown") != std::string::npos,
                        "Unexpected reason: " << error);
}

BOOST_AUTO_TEST_CASE(test_root_is_refused) {
    ECF_NAME_THIS_TEST();

    std::string error;
    BOOST_CHECK(!resolve_spawn_identity("root", error));
    BOOST_CHECK_MESSAGE(error.find("root") != std::string::npos, "Unexpected reason: " << error);
}

BOOST_AUTO_TEST_CASE(test_the_current_user_resolves_to_its_account) {
    ECF_NAME_THIS_TEST();

    if (geteuid() == 0) {
        std::cout << "  Skipped: running as root, whose identity is refused by design\n";
        return;
    }
    struct passwd* me = getpwuid(geteuid());
    BOOST_REQUIRE_MESSAGE(me && me->pw_name, "Expected the current user in the account database");

    std::string error;
    auto identity = resolve_spawn_identity(me->pw_name, error);
    BOOST_REQUIRE_MESSAGE(identity, "Expected the current user to resolve, got: " << error);
    BOOST_CHECK(error.empty());
    BOOST_CHECK_EQUAL(identity->name, me->pw_name);
    BOOST_CHECK_EQUAL(identity->uid, geteuid());
    BOOST_CHECK_EQUAL(identity->gid, me->pw_gid);
    BOOST_CHECK_MESSAGE(!identity->home.empty(), "Expected a home directory");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
