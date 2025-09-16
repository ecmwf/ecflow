/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Permissions)

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
    s->addVariable(Variable(ecf::environment::ECF_PERMISSIONS, "u1:rwx,u2:rw"));
    auto f = s->add_family("f1");
    f->addVariable(Variable(ecf::environment::ECF_PERMISSIONS, "u2:rx,u3:rw"));
    auto t = f->add_task("t1");

    d.server_state().add_or_update_server_variable(ecf::environment::ECF_PERMISSIONS, "a:rwxos");

    AuthorisationService service = AuthorisationService::load_permissions_from_nodes().value();

    auto a  = Username{"a"};
    auto u1 = Username{"u1"};
    auto u2 = Username{"u2"};
    auto u3 = Username{"u3"};

    {
        auto selected = service.permissions_at(d, "/s1"s);

        BOOST_CHECK(selected.allows(a, Allowed::READ));
        BOOST_CHECK(selected.allows(a, Allowed::WRITE));
        BOOST_CHECK(selected.allows(a, Allowed::EXECUTE));
        BOOST_CHECK(selected.allows(a, Allowed::OWNER));
        BOOST_CHECK(selected.allows(a, Allowed::STICKY));

        BOOST_CHECK(selected.allows(u1, Allowed::READ));
        BOOST_CHECK(selected.allows(u1, Allowed::WRITE));
        BOOST_CHECK(selected.allows(u1, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u1, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u1, Allowed::STICKY));

        BOOST_CHECK(selected.allows(u2, Allowed::READ));
        BOOST_CHECK(selected.allows(u2, Allowed::WRITE));
        BOOST_CHECK(!selected.allows(u2, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u2, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u2, Allowed::STICKY));

        BOOST_CHECK(!selected.allows(u3, Allowed::READ));
        BOOST_CHECK(!selected.allows(u3, Allowed::WRITE));
        BOOST_CHECK(!selected.allows(u3, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u3, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u3, Allowed::STICKY));
    }

    {
        auto selected = service.permissions_at(d, "/s1/f1"s);

        BOOST_CHECK(selected.allows(a, Allowed::READ));
        BOOST_CHECK(selected.allows(a, Allowed::WRITE));
        BOOST_CHECK(selected.allows(a, Allowed::EXECUTE));
        BOOST_CHECK(selected.allows(a, Allowed::OWNER));
        BOOST_CHECK(selected.allows(a, Allowed::STICKY));

        BOOST_CHECK(!selected.allows(u1, Allowed::READ));
        BOOST_CHECK(!selected.allows(u1, Allowed::WRITE));
        BOOST_CHECK(!selected.allows(u1, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u1, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u1, Allowed::STICKY));

        BOOST_CHECK(selected.allows(u2, Allowed::READ));
        BOOST_CHECK(!selected.allows(u2, Allowed::WRITE));
        BOOST_CHECK(!selected.allows(u2, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u2, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u2, Allowed::STICKY));

        BOOST_CHECK(!selected.allows(u3, Allowed::READ));
        BOOST_CHECK(!selected.allows(u3, Allowed::WRITE));
        BOOST_CHECK(!selected.allows(u3, Allowed::EXECUTE));
        BOOST_CHECK(!selected.allows(u3, Allowed::OWNER));
        BOOST_CHECK(!selected.allows(u3, Allowed::STICKY));
    }
}

BOOST_AUTO_TEST_CASE(can_calculate_permission_superseeding_basic_rules) {
    using namespace ecf;

    auto a  = Username{"a"};
    auto u1 = Username{"u1"};
    auto u2 = Username{"u2"};

    auto p = Permissions::make_from_variable("a:rws,u1:rw");
    BOOST_CHECK(p.allows(a, Allowed::READ));
    BOOST_CHECK(p.allows(a, Allowed::WRITE));
    BOOST_CHECK(!p.allows(a, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(a, Allowed::OWNER));
    BOOST_CHECK(p.allows(a, Allowed::STICKY));
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("a:r,u1:rwx,u2:rw");
    BOOST_CHECK(q.allows(a, Allowed::READ));
    BOOST_CHECK(!q.allows(a, Allowed::WRITE));
    BOOST_CHECK(!q.allows(a, Allowed::EXECUTE));
    BOOST_CHECK(!q.allows(a, Allowed::OWNER));
    BOOST_CHECK(!q.allows(a, Allowed::STICKY));
    BOOST_CHECK(q.allows(u1, Allowed::READ));
    BOOST_CHECK(q.allows(u1, Allowed::WRITE));
    BOOST_CHECK(q.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!q.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!q.allows(u1, Allowed::STICKY));
    BOOST_CHECK(q.allows(u2, Allowed::READ));
    BOOST_CHECK(q.allows(u2, Allowed::WRITE));
    BOOST_CHECK(!q.allows(u2, Allowed::EXECUTE));
    BOOST_CHECK(!q.allows(u2, Allowed::OWNER));
    BOOST_CHECK(!q.allows(u2, Allowed::STICKY));

    auto r = Permissions::combine_supersede(p, q);
    BOOST_CHECK(r.allows(a, Allowed::READ));
    BOOST_CHECK(r.allows(a, Allowed::WRITE));
    BOOST_CHECK(!r.allows(a, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(a, Allowed::OWNER));
    BOOST_CHECK(r.allows(a, Allowed::STICKY));
    BOOST_CHECK(r.allows(u1, Allowed::READ));
    BOOST_CHECK(r.allows(u1, Allowed::WRITE));
    BOOST_CHECK(r.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!r.allows(u1, Allowed::STICKY));
    BOOST_CHECK(r.allows(u2, Allowed::READ));
    BOOST_CHECK(r.allows(u2, Allowed::WRITE));
    BOOST_CHECK(!r.allows(u2, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u2, Allowed::OWNER));
    BOOST_CHECK(!r.allows(u2, Allowed::STICKY));
}

BOOST_AUTO_TEST_CASE(can_calculate_permission_overriding_basic_rules) {
    using namespace ecf;

    auto u1 = Username{"u1"};
    auto u2 = Username{"u2"};

    auto p = Permissions::make_from_variable("u1:rwx");
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:rw,u2:rwxo");
    BOOST_CHECK(q.allows(u1, Allowed::READ));
    BOOST_CHECK(q.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!q.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!q.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!q.allows(u1, Allowed::STICKY));

    auto r = Permissions::combine_override(p, q);
    BOOST_CHECK(r.allows(u1, Allowed::READ));
    BOOST_CHECK(r.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!r.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!r.allows(u1, Allowed::STICKY));

    BOOST_CHECK(!r.allows(u2, Allowed::READ));
    BOOST_CHECK(!r.allows(u2, Allowed::WRITE));
    BOOST_CHECK(!r.allows(u2, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u2, Allowed::STICKY));
}

BOOST_AUTO_TEST_CASE(can_calculate_permission_overriding_does_not_extend_allowances) {
    using namespace ecf;

    auto u1 = Username{"u1"};

    auto p = Permissions::make_from_variable("u1:rw");
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:rwxo");
    BOOST_CHECK(q.allows(u1, Allowed::READ));
    BOOST_CHECK(q.allows(u1, Allowed::WRITE));
    BOOST_CHECK(q.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(q.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!q.allows(u1, Allowed::STICKY));

    auto r = Permissions::combine_override(p, q);
    BOOST_CHECK(r.allows(u1, Allowed::READ));
    BOOST_CHECK(r.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!r.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!r.allows(u1, Allowed::STICKY));
}

BOOST_AUTO_TEST_CASE(can_calculate_permission_overriding_keeps_sticky_allowances) {
    using namespace ecf;

    auto u1 = Username{"u1"};

    auto p = Permissions::make_from_variable("u1:rwxs");
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:r");
    BOOST_CHECK(q.allows(u1, Allowed::READ));
    BOOST_CHECK(!q.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!q.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!q.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!q.allows(u1, Allowed::STICKY));

    auto r = Permissions::combine_override(p, q);
    BOOST_CHECK(r.allows(u1, Allowed::READ));
    BOOST_CHECK(r.allows(u1, Allowed::WRITE));
    BOOST_CHECK(r.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!r.allows(u1, Allowed::OWNER));
    BOOST_CHECK(r.allows(u1, Allowed::STICKY));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
