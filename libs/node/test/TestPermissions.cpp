// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

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

    using namespace ecf::test::scaffold;

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

    using namespace ecf::test::scaffold;

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

    auto a  = Identity::make_secure_user(Username{"a"}.value());
    auto u1 = Identity::make_secure_user(Username{"u1"}.value());
    auto u2 = Identity::make_secure_user(Username{"u2"}.value());
    auto u3 = Identity::make_secure_user(Username{"u3"}.value());

    {
        const auto& path = "/"s;

        BOOST_CHECK(service.allows(a, d, path, Allowed::READ));
        BOOST_CHECK(service.allows(a, d, path, Allowed::WRITE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::EXECUTE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::OWNER));
        BOOST_CHECK(service.allows(a, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u1, d, path, Allowed::READ));    //
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::WRITE));   //
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::EXECUTE)); //
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u2, d, path, Allowed::READ));  //
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::WRITE)); //
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u3, d, path, Allowed::READ));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::STICKY));
    }

    {
        const auto& path = "/s1"s;

        BOOST_CHECK(service.allows(a, d, path, Allowed::READ));
        BOOST_CHECK(service.allows(a, d, path, Allowed::WRITE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::EXECUTE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::OWNER));
        BOOST_CHECK(service.allows(a, d, path, Allowed::STICKY));

        BOOST_CHECK(service.allows(u1, d, path, Allowed::READ));    //
        BOOST_CHECK(service.allows(u1, d, path, Allowed::WRITE));   //
        BOOST_CHECK(service.allows(u1, d, path, Allowed::EXECUTE)); //
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::STICKY));

        BOOST_CHECK(service.allows(u2, d, path, Allowed::READ));  //
        BOOST_CHECK(service.allows(u2, d, path, Allowed::WRITE)); //
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u3, d, path, Allowed::READ));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::STICKY));
    }

    {
        const auto& path = "/s1/f1"s;

        BOOST_CHECK(service.allows(a, d, path, Allowed::READ));
        BOOST_CHECK(service.allows(a, d, path, Allowed::WRITE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::EXECUTE));
        BOOST_CHECK(service.allows(a, d, path, Allowed::OWNER));
        BOOST_CHECK(service.allows(a, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u1, d, path, Allowed::READ));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u1, d, path, Allowed::STICKY));

        BOOST_CHECK(service.allows(u2, d, path, Allowed::READ)); //
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u2, d, path, Allowed::STICKY));

        BOOST_CHECK(!service.allows(u3, d, path, Allowed::READ));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::EXECUTE));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::OWNER));
        BOOST_CHECK(!service.allows(u3, d, path, Allowed::STICKY));
    }
}

BOOST_AUTO_TEST_CASE(t1_suite_without_permissions_drops_non_sticky_server_users) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;
    using namespace std::string_literals;

    // Server grants 'admin' sticky rights and 'ops' non-sticky rights; the suite defines no ECF_PERMISSIONS
    Defs d;
    auto s = d.add_suite("s1");
    auto f = s->add_family("f1");
    f->add_task("t1");
    d.server_state().add_or_update_server_variable(ecf::environment::ECF_PERMISSIONS, "admin:rwxos,ops:rw");

    AuthorisationService service = AuthorisationService::load_permissions_from_nodes().value();

    auto admin = Identity::make_secure_user(Username{"admin"}.value());
    auto ops   = Identity::make_secure_user(Username{"ops"}.value());

    // At server level, both users are allowed
    BOOST_CHECK(service.allows(admin, d, "/"s, Allowed::READ));
    BOOST_CHECK(service.allows(ops, d, "/"s, Allowed::READ));
    BOOST_CHECK(service.allows(ops, d, "/"s, Allowed::WRITE));

    // Below the suite, only the sticky user survives (T1)
    for (auto&& path : {"/s1"s, "/s1/f1"s, "/s1/f1/t1"s}) {
        BOOST_CHECK(service.allows(admin, d, path, Allowed::READ));
        BOOST_CHECK(service.allows(admin, d, path, Allowed::WRITE));
        BOOST_CHECK(!service.allows(ops, d, path, Allowed::READ));
        BOOST_CHECK(!service.allows(ops, d, path, Allowed::WRITE));
    }
}

BOOST_AUTO_TEST_CASE(t2_collapsed_active_permissions_allow_everyone) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;
    using namespace std::string_literals;

    // Server grants 'ops' (non-sticky); suite lists only 'alice'; family lists only 'bob' (never active above)
    Defs d;
    auto s = d.add_suite("s1");
    s->addVariable(Variable(ecf::environment::ECF_PERMISSIONS, "alice:rw"));
    auto f = s->add_family("f1");
    f->addVariable(Variable(ecf::environment::ECF_PERMISSIONS, "bob:r"));
    f->add_task("t1");
    d.server_state().add_or_update_server_variable(ecf::environment::ECF_PERMISSIONS, "ops:rw");

    AuthorisationService service = AuthorisationService::load_permissions_from_nodes().value();

    auto ops      = Identity::make_secure_user(Username{"ops"}.value());
    auto alice    = Identity::make_secure_user(Username{"alice"}.value());
    auto bob      = Identity::make_secure_user(Username{"bob"}.value());
    auto stranger = Identity::make_secure_user(Username{"stranger"}.value());

    // At the suite, only alice is active
    BOOST_CHECK(service.allows(alice, d, "/s1"s, Allowed::WRITE));
    BOOST_CHECK(!service.allows(ops, d, "/s1"s, Allowed::READ));
    BOOST_CHECK(!service.allows(bob, d, "/s1"s, Allowed::READ));
    BOOST_CHECK(!service.allows(stranger, d, "/s1"s, Allowed::READ));

    // At the family, the active set collapses to empty, which is treated as "no rules" (T2)
    for (auto&& path : {"/s1/f1"s, "/s1/f1/t1"s}) {
        for (auto&& who : {ops, alice, bob, stranger}) {
            BOOST_CHECK(service.allows(who, d, path, Allowed::READ));
            BOOST_CHECK(service.allows(who, d, path, Allowed::WRITE));
            BOOST_CHECK(service.allows(who, d, path, Allowed::EXECUTE));
            BOOST_CHECK(service.allows(who, d, path, Allowed::OWNER));
        }
    }
}

BOOST_AUTO_TEST_CASE(can_calculate_permission_superseeding_basic_rules) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;

    auto a  = Username{"a"};
    auto u1 = Username{"u1"};
    auto u2 = Username{"u2"};

    auto p = Permissions::make_from_variable("a:rws,u1:rw").value();
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

    auto q = Permissions::make_from_variable("a:r,u1:rwx,u2:rw").value();
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
    ECF_NAME_THIS_TEST();
    using namespace ecf;

    auto u1 = Username{"u1"};
    auto u2 = Username{"u2"};

    auto p = Permissions::make_from_variable("u1:rwx").value();
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:rw,u2:rwxo").value();
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
    ECF_NAME_THIS_TEST();
    using namespace ecf;

    auto u1 = Username{"u1"};

    auto p = Permissions::make_from_variable("u1:rw").value();
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(!p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(!p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:rwxo").value();
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
    ECF_NAME_THIS_TEST();
    using namespace ecf;

    auto u1 = Username{"u1"};

    auto p = Permissions::make_from_variable("u1:rwxs").value();
    BOOST_CHECK(p.allows(u1, Allowed::READ));
    BOOST_CHECK(p.allows(u1, Allowed::WRITE));
    BOOST_CHECK(p.allows(u1, Allowed::EXECUTE));
    BOOST_CHECK(!p.allows(u1, Allowed::OWNER));
    BOOST_CHECK(p.allows(u1, Allowed::STICKY));

    auto q = Permissions::make_from_variable("u1:r").value();
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

BOOST_AUTO_TEST_CASE(can_detect_valid_permission_values) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;
    using namespace std::string_literals;

    std::array perms = {"a:RWXS,b:rwx"s};
    for (const auto& actual : perms) {
        BOOST_CHECK_MESSAGE(Permissions::make_from_variable(actual).ok(),
                            ">" << actual << "< has valid permission format");
    }
}

BOOST_AUTO_TEST_CASE(can_detect_invalid_permission_values) {
    ECF_NAME_THIS_TEST();
    using namespace ecf;
    using namespace std::string_literals;

    std::array perms = {":rwxs,:rwx"s,
                        "a:,b:"s,
                        "a:rwxs;b:rwx"s,
                        "a:rwxZ"s,
                        "a:rwx,b:rwx!"s,
                        "a:rwx,b:rwx,"s,
                        ",a:rwx,b:rwx,"s,
                        "a:rwx,,b:rwx"s,
                        "a:rwx,x,b:rwx"s};
    for (const auto& actual : perms) {
        BOOST_CHECK_MESSAGE(!Permissions::make_from_variable(actual).ok(),
                            ">" << actual << "< has invalid permission format");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
