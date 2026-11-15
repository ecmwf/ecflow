// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/ArchivedNodes.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The walk of a subtree with its archived descendants: nested archives are read, nothing changes (the change numbers
/// of the server included), and an archive that cannot be read is reported, so that the decision whether p is
/// required fails closed.
///

namespace {

///
/// @brief Holds definitions, on the server side, whose archives go to a directory of the test: suite s with family
///        f (task u naming alice, and family g with task t naming alice), f and g archived.
///
struct Fixture
{
    ecf::test::scaffold::Directory home = ecf::test::scaffold::MakeDirectory{}.create();
    ecf::test::scaffold::TestLog log{(home.path() / "test_archived_nodes.log").string()};
    defs_ptr defs = Defs::create();
    family_ptr f;

    // The change numbers of the process, restored afterwards: other tests count from them
    Ecf::counter_t state_change_no  = Ecf::state_change_no();
    Ecf::counter_t modify_change_no = Ecf::modify_change_no();

    Fixture() {
        Ecf::set_server(true);
        defs->server_state().add_or_update_user_variables(ecf::environment::ECF_HOME, home.path().string());
        defs->server_state().set_permissions(Permissions::parse_cli("alice:rwxop"));
        f = defs->add_suite("s")->add_family("f");
        f->add_task("u")->add_permissions(Permissions::parse_cli("alice:rwxp"));
        f->add_family("g")->add_task("t")->add_permissions(Permissions::parse_cli("alice:rp"));
        f->findFamily("g")->archive();
        f->archive();
    }
    ~Fixture() {
        Ecf::set_server(false);
        Ecf::set_state_change_no(state_change_no);
        Ecf::set_modify_change_no(modify_change_no);
    }
    Fixture(const Fixture&)            = delete;
    Fixture& operator=(const Fixture&) = delete;

    ///
    /// @brief Returns the paths visited by a walk of /s, and whether every archive was read.
    ///
    /// @return The paths, in the order of the walk, and the result of the walk.
    ///
    std::pair<std::vector<std::string>, bool> walk() const {
        std::vector<std::string> paths;
        const bool complete = ecf::for_each_node(*defs->findAbsNode("/s"),
                                                 [&paths](const Node& node) { paths.push_back(node.absNodePath()); });
        return {paths, complete};
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_FIXTURE_TEST_SUITE(T_ArchivedNodes, Fixture)

///
/// @brief Verifies that the walk reads nested archives, and leaves the change numbers and the definitions unchanged.
///
BOOST_AUTO_TEST_CASE(test_walk_reads_nested_archives) {
    ECF_NAME_THIS_TEST();

    BOOST_REQUIRE(f->children().empty());
    const auto state_no          = Ecf::state_change_no();
    const auto modify_no         = Ecf::modify_change_no();
    const auto [paths, complete] = walk();
    BOOST_CHECK(complete);
    BOOST_CHECK(paths == (std::vector<std::string>{"/s", "/s/f", "/s/f/u", "/s/f/g", "/s/f/g/t"}));
    BOOST_CHECK(ecf::carries_permissions(*f));
    BOOST_CHECK_EQUAL(Ecf::state_change_no(), state_no);
    BOOST_CHECK_EQUAL(Ecf::modify_change_no(), modify_no);
    BOOST_CHECK(f->children().empty());
    BOOST_CHECK(Ecf::server());
}

///
/// @brief Verifies that a missing or invalid archive, or one holding the path as a task, is reported, and makes
///        carries_permissions() fail closed, without changing the change numbers.
///
BOOST_AUTO_TEST_CASE(test_unreadable_archives_fail_closed) {
    ECF_NAME_THIS_TEST();

    const auto archive = f->archive_path();
    for (const std::string kind : {"invalid", "wrong type", "missing"}) {
        BOOST_TEST_CONTEXT(kind << " archive") {
            if (kind == "missing") {
                fs::remove(archive);
            }
            else if (kind == "wrong type") {
                // Parses, and holds /s/f, but as a task
                std::ofstream(archive) << "suite s\n  task f\nendsuite\n";
            }
            else {
                std::ofstream(archive) << "this is not a definition\n";
            }
            const auto state_no          = Ecf::state_change_no();
            const auto modify_no         = Ecf::modify_change_no();
            const auto [paths, complete] = walk();
            BOOST_CHECK(!complete);
            BOOST_CHECK(paths == (std::vector<std::string>{"/s", "/s/f"}));
            BOOST_CHECK(ecf::carries_permissions(*f));
            BOOST_CHECK_EQUAL(Ecf::state_change_no(), state_no);
            BOOST_CHECK_EQUAL(Ecf::modify_change_no(), modify_no);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
