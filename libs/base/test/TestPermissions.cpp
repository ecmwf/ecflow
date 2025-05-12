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

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/server/AuthorisationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "harness/MockServer.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_Perms)

BOOST_AUTO_TEST_CASE(test_file_automatic_name_exists) {
    ECF_NAME_THIS_TEST();

    std::vector<fs::path> paths;
    for (int i = 0; i < 10; i++) {
        WithTestFile file;

        auto path = fs::absolute(file.path());
        paths.push_back(path);

        BOOST_CHECK(fs::exists(path));
    }
    for (const auto& path : paths) {
        BOOST_CHECK(!fs::exists(path));
    }
}

BOOST_AUTO_TEST_CASE(test_file_automatic_prefix_name_exists) {
    ECF_NAME_THIS_TEST();

    std::vector<fs::path> paths;
    for (int i = 0; i < 10; i++) {
        auto prefix = "test_file_" + std::to_string(i);
        WithTestFile file{AutomaticTestFile{prefix}};

        auto path = file.path();
        paths.push_back(path);

        BOOST_CHECK(fs::exists(path));
    }
    for (const auto& path : paths) {
        BOOST_CHECK(!fs::exists(path));
    }
}

BOOST_AUTO_TEST_CASE(can_do_permissions) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;
    using namespace std::string_literals;

    Defs d;
    auto s = d.add_suite("s1");
    s->addVariable(Variable("PERMISSIONS", "u1:rwx,u2:rw"));
    auto f = s->add_family("f1");
    f->addVariable(Variable("PERMISSIONS", "u2:r,u3:rw"));
    auto t = f->add_task("t1");

    MockServer server(&d);
    d.server_state().add_or_update_server_variable("PERMISSIONS", "a:rwxo");

    AuthorisationService service = AuthorisationService::load_permissions_from_nodes().value();

    {
        auto selected = service.permissions_at(d, "/s1/f1/t1"s);

        BOOST_REQUIRE(selected.allows(Username{"a"}, Allowed::READ));
        BOOST_REQUIRE(selected.allows(Username{"a"}, Allowed::WRITE));
        BOOST_REQUIRE(selected.allows(Username{"a"}, Allowed::EXECUTE));
        BOOST_REQUIRE(selected.allows(Username{"a"}, Allowed::OWNER));

        BOOST_REQUIRE(selected.allows(Username{"u1"}, Allowed::READ));
        BOOST_REQUIRE(selected.allows(Username{"u1"}, Allowed::WRITE));
        BOOST_REQUIRE(selected.allows(Username{"u1"}, Allowed::EXECUTE));
        BOOST_REQUIRE(!selected.allows(Username{"u1"}, Allowed::OWNER));

        BOOST_REQUIRE(selected.allows(Username{"u2"}, Allowed::READ));
        BOOST_REQUIRE(!selected.allows(Username{"u2"}, Allowed::WRITE));
        BOOST_REQUIRE(!selected.allows(Username{"u2"}, Allowed::EXECUTE));
        BOOST_REQUIRE(!selected.allows(Username{"u2"}, Allowed::OWNER));

        BOOST_REQUIRE(!selected.allows(Username{"u3"}, Allowed::READ));
        BOOST_REQUIRE(!selected.allows(Username{"u3"}, Allowed::WRITE));
        BOOST_REQUIRE(!selected.allows(Username{"u3"}, Allowed::EXECUTE));
        BOOST_REQUIRE(!selected.allows(Username{"u3"}, Allowed::OWNER));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
