// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/PermissionsChange.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using ecf::Permissions;

///
/// The restoration of archived nodes (D3.5, D3.6), the preservation of attributes through begin, requeue and reset
/// (R3.6), and the check of every attribute of a checkpoint (R3.5, D11.4).
///

namespace {

///
/// @brief Holds definitions whose archives go to a directory of the test, and the log of the test.
///
struct Fixture
{
    ecf::test::scaffold::Directory home = ecf::test::scaffold::MakeDirectory{}.create();
    ecf::test::scaffold::TestLog log{(home.path() / "test_permissions_restore.log").string()};
    defs_ptr defs = Defs::create();

    Fixture() { defs->server_state().add_or_update_user_variables(ecf::environment::ECF_HOME, home.path().string()); }

    ///
    /// @brief Returns the content of the log of the test, flushed.
    ///
    /// @return The content.
    ///
    std::string log_text() const {
        { ecf::LogFlusher flusher; }
        std::ifstream file(ecf::Log::instance()->path());
        std::stringstream content;
        content << file.rdbuf();
        return content.str();
    }
};

///
/// @brief Returns the attribute at a path, in the command-line form.
///
/// @param[in] defs The definitions
/// @param[in] path The path of a node
/// @return The attribute; "-" when the node has none, "missing" when no node has the path.
///
std::string at(const defs_ptr& defs, const std::string& path) {
    const auto node = defs->findAbsNode(path);
    if (!node) {
        return "missing";
    }
    return node->permissions() ? node->permissions()->to_cli() : "-";
}

///
/// @brief Changes an attribute through the validation of changes.
///
/// @param[in] defs The definitions
/// @param[in] path The path of the node
/// @param[in] value The attribute, in the command-line form
/// @throws std::runtime_error when the change is rejected.
///
void change(const defs_ptr& defs, const std::string& path, const std::string& value) {
    ecf::PermissionsChange change(*defs);
    change.set(*defs->findAbsNode(path), Permissions::parse_cli(value));
    change.commit("admin@host");
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_FIXTURE_TEST_SUITE(T_PermissionsRestore, Fixture)

///
/// @brief Verifies that restoring archived nodes removes the entries of users no longer listed above, and the
///        attributes left empty, logging each removal (D3.5); explicit and automatic restores alike.
///
BOOST_AUTO_TEST_CASE(test_restore_removes_users_no_longer_listed) {
    ECF_NAME_THIS_TEST();

    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r"));
    auto s = defs->add_suite("s");
    s->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
    auto f = s->add_family("f");
    f->add_task("t")->add_permissions(Permissions::parse_cli("bob:r"));
    f->add_task("u")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));

    for (const bool requested : {true, false}) {
        BOOST_TEST_CONTEXT((requested ? "explicit" : "automatic") << " restore") {
            if (requested) {
                change(defs, "/s", "alice:rwxp,bob:r");
            }
            else {
                // Back to the starting point: bob listed by /s, and in the archived tasks
                f->add_task("t")->add_permissions(Permissions::parse_cli("bob:r"));
                f->add_task("u")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
                change(defs, "/s", "alice:rwxp,bob:r");
            }
            f->archive();
            BOOST_REQUIRE(f->children().empty());

            // bob is removed from /s while f is archived: the cascade does not reach the archive
            change(defs, "/s", "alice:rwxp");

            f->restore(true,
                       requested ? NodeContainer::RestoreTrigger::Requested : NodeContainer::RestoreTrigger::Automatic);
            BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "-");
            BOOST_CHECK_EQUAL(at(defs, "/s/f/u"), "alice:rwxp");
            const auto text = log_text();
            BOOST_CHECK_MESSAGE(
                text.find("permissions /s/f/t bob:r -> (none) (removal of users above) by restore of /s/f") !=
                    std::string::npos,
                text);
            BOOST_CHECK_MESSAGE(text.find("permissions /s/f/u alice:rwxp,bob:r -> alice:rwxp (removal of users above) "
                                          "by restore of /s/f") != std::string::npos,
                                text);

            // For the next round
            defs->deleteChild(f->findTask("t").get());
            defs->deleteChild(f->findTask("u").get());
        }
    }
}

///
/// @brief Verifies that a restore leaving a node without any holder of p is accepted, with a warning naming the node
///        (D3.5, which takes precedence over D4.6).
///
BOOST_AUTO_TEST_CASE(test_restore_accepts_a_lockout_with_a_warning) {
    ECF_NAME_THIS_TEST();

    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,carol:rwxp"));
    auto s = defs->add_suite("s");
    s->add_permissions(Permissions::parse_cli("alice:rwxp,carol:rwxp"));
    auto f = s->add_family("f");
    f->add_family("g")->add_permissions(Permissions::parse_cli("carol:rwxp"));
    f->archive();

    // carol keeps r only at /s: below it, nobody holds p at /s/f/g once restored
    change(defs, "/s", "alice:rwxp,carol:r");
    BOOST_CHECK_NO_THROW(f->restore());
    BOOST_CHECK_EQUAL(at(defs, "/s/f/g"), "carol:rwxp");
    const auto text = log_text();
    BOOST_CHECK_MESSAGE(
        text.find("WAR:") != std::string::npos &&
            text.find("restore of /s/f: nobody is able to change the Permissions attribute of /s/f/g") !=
                std::string::npos,
        text);
}

///
/// @brief Verifies the restore of archived nodes carrying attributes on a server without a server attribute (D3.6):
///        an explicit restore fails, leaving the archive; an automatic one strips the attributes, with a warning.
///
BOOST_AUTO_TEST_CASE(test_restore_on_an_unrestricted_server) {
    ECF_NAME_THIS_TEST();

    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp"));
    auto s = defs->add_suite("s");
    auto f = s->add_family("f");
    f->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp"));
    f->add_task("u");
    f->archive();
    const auto archive = f->archive_path();
    defs->server_state().clear_permissions();

    BOOST_CHECK_THROW(f->restore(true, NodeContainer::RestoreTrigger::Requested), std::runtime_error);
    BOOST_CHECK(f->get_flag().is_set(ecf::Flag::ARCHIVED));
    BOOST_CHECK(f->children().empty());
    BOOST_CHECK(fs::exists(archive));

    BOOST_CHECK_NO_THROW(f->restore(true, NodeContainer::RestoreTrigger::Automatic));
    BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), "-");
    BOOST_CHECK_EQUAL(at(defs, "/s/f/u"), "-");
    const auto text = log_text();
    BOOST_CHECK_MESSAGE(text.find("restore of /s/f: Permissions attributes removed, the server having no server "
                                  "Permissions attribute: /s/f/t") != std::string::npos,
                        text);
}

///
/// @brief Verifies the automatic restores (autorestore, begin, requeue) of archived nodes carrying attributes: on a
///        server without a server attribute, the attributes are stripped, with a warning (D3.6); with one, the entries
///        of users no longer listed above are removed (D3.5).
///
BOOST_AUTO_TEST_CASE(test_automatic_restores) {
    ECF_NAME_THIS_TEST();

    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r"));
    auto s = defs->add_suite("s");
    s->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
    auto f = s->add_family("f");
    auto r = s->add_task("r");
    r->add_autorestore(ecf::AutoRestoreAttr(std::vector<std::string>{"/s/f"}));

    // Archives f holding t, which names bob, then removes bob from /s, or the server attribute
    const auto archive = [&](bool unrestricted) {
        f->add_task("t")->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
        defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r"));
        change(defs, "/s", "alice:rwxp,bob:r");
        f->archive();
        BOOST_REQUIRE(f->children().empty());
        if (unrestricted) {
            s->set_permissions(nullptr);
            defs->server_state().clear_permissions();
        }
        else {
            change(defs, "/s", "alice:rwxp");
        }
    };
    const std::vector<std::pair<std::string, std::function<void()>>> triggers{
        {"autorestore", [&]() { r->get_autorestore()->do_autorestore(); }},
        {"begin",
         [&]() {
             s->reset_begin(); // a suite is begun once
             s->begin();
         }},
        {"requeue",
         [&]() {
             Node::Requeue_args args;
             s->requeue(args);
         }},
    };
    for (const auto& [name, trigger] : triggers) {
        for (const bool unrestricted : {true, false}) {
            BOOST_TEST_CONTEXT(name << (unrestricted ? ", without a server attribute" : ", with a server attribute")) {
                archive(unrestricted);
                const auto logged = log_text().size();
                trigger();
                BOOST_REQUIRE_MESSAGE(!f->get_flag().is_set(ecf::Flag::ARCHIVED), log_text().substr(logged));
                BOOST_CHECK_EQUAL(at(defs, "/s/f/t"), unrestricted ? "-" : "alice:rwxp");
                const auto text = log_text().substr(logged); // what this restore wrote
                if (unrestricted) {
                    BOOST_CHECK_MESSAGE(text.find("restore of /s/f: Permissions attributes removed, the server "
                                                  "having no server Permissions attribute: /s/f/t") !=
                                            std::string::npos,
                                        text);
                }
                else {
                    BOOST_CHECK_MESSAGE(text.find("permissions /s/f/t alice:rwxp,bob:r -> alice:rwxp (removal of users "
                                                  "above) by restore of /s/f") != std::string::npos,
                                        text);
                }
                defs->deleteChild(f->findTask("t").get());
                s->set_permissions(nullptr);
                s->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
            }
        }
    }
}

///
/// @brief Verifies that begin, requeue and reset leave the attributes unchanged (R3.6).
///
BOOST_AUTO_TEST_CASE(test_attributes_preserved_by_begin_requeue_and_reset) {
    ECF_NAME_THIS_TEST();

    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r"));
    auto s = defs->add_suite("s");
    s->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
    auto f = s->add_family("f");
    f->add_permissions(Permissions::parse_cli("alice:rwxp"));
    f->add_task("t")->add_permissions(Permissions::parse_cli("alice:rp"));

    const auto check = [&](const std::string& step) {
        BOOST_CHECK_MESSAGE(at(defs, "/s") == "alice:rwxp,bob:r", step);
        BOOST_CHECK_MESSAGE(at(defs, "/s/f") == "alice:rwxp", step);
        BOOST_CHECK_MESSAGE(at(defs, "/s/f/t") == "alice:rp", step);
        BOOST_REQUIRE_MESSAGE(defs->server_state().permissions(), step);
        BOOST_CHECK_MESSAGE(defs->server_state().permissions()->to_cli() == "admin:rwxlop,alice:rwxp,bob:r", step);
    };
    s->begin();
    check("begin");
    Node::Requeue_args args;
    s->requeue(args);
    check("requeue");
    s->reset();
    check("reset");
}

///
/// @brief Verifies the check of every attribute of definitions loaded from a checkpoint (R3.5, D11.4), under the
///        server attribute of the checkpoint or of the configuration file.
///
BOOST_AUTO_TEST_CASE(test_check_definitions) {
    ECF_NAME_THIS_TEST();

    const auto configured = Permissions::parse_cli("admin:rwxlop,alice:rwxp,bob:r");
    auto s                = defs->add_suite("s");
    s->add_permissions(Permissions::parse_cli("alice:rwxp,bob:r"));
    auto f = s->add_family("f");
    f->add_permissions(Permissions::parse_cli("alice:rwxp"));

    // Valid under the configured attribute; without any, node attributes are invalid (D2.20)
    BOOST_CHECK_NO_THROW(ecf::PermissionsChange(*defs).check_definitions(&configured));
    BOOST_CHECK_EXCEPTION(
        ecf::PermissionsChange(*defs).check_definitions(nullptr), std::runtime_error, [](const std::runtime_error& e) {
            return std::string(e.what()).find("/s requires a server Permissions attribute") != std::string::npos;
        });

    // A user not listed above (D2.15, D2.18)
    f->add_task("t")->add_permissions(Permissions::parse_cli("bob:r"));
    BOOST_CHECK_EXCEPTION(ecf::PermissionsChange(*defs).check_definitions(&configured),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              return std::string(e.what()).find("/s/f/t names the user 'bob'") != std::string::npos;
                          });

    // The server attribute of the checkpoint applies when present; the definitions are left unchanged
    defs->server_state().set_permissions(Permissions::parse_cli("admin:rwxlop,alice:rwxp"));
    f->findTask("t")->set_permissions(nullptr);
    BOOST_CHECK_EXCEPTION(ecf::PermissionsChange(*defs).check_definitions(&configured),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              return std::string(e.what()).find("/s names the user 'bob'") != std::string::npos;
                          });
    BOOST_CHECK_EQUAL(defs->server_state().permissions()->to_cli(), "admin:rwxlop,alice:rwxp");
}

///
/// @brief Verifies the check of a checkpoint on a server without a server attribute (D2.20): the first node carrying
///        an attribute, in the order of a walk of the tree (aliases included), is named; definitions without
///        attributes are accepted.
///
BOOST_AUTO_TEST_CASE(test_check_definitions_without_server_attribute) {
    ECF_NAME_THIS_TEST();

    auto s = defs->add_suite("s");
    auto a = s->add_family("a");
    auto t = a->add_task("t");
    auto b = s->add_family("b");
    defs->add_suite("u");
    BOOST_CHECK_NO_THROW(ecf::PermissionsChange(*defs).check_definitions(nullptr));

    const auto attribute = Permissions::parse_cli("alice:rwxp");
    b->add_permissions(attribute);
    t->add_alias_only()->add_permissions(attribute);
    defs->findSuite("u")->add_permissions(attribute);
    for (const auto& [expected, cleared] : std::vector<std::pair<std::string, Node*>>{
             {"/s/a/t/alias0", t->find_alias("alias0").get()}, {"/s/b", b.get()}, {"/u", nullptr}}) {
        BOOST_TEST_CONTEXT(expected) {
            try {
                ecf::PermissionsChange(*defs).check_definitions(nullptr);
                BOOST_ERROR("not refused");
            }
            catch (const ecf::InvalidPermissions& e) {
                BOOST_CHECK_EQUAL(e.path(), expected);
                BOOST_CHECK_EQUAL(std::string(e.what()),
                                  "The Permissions attribute of " + expected +
                                      " requires a server Permissions attribute");
            }
            if (cleared) {
                cleared->set_permissions(nullptr);
            }
        }
    }
}

///
/// @brief Verifies the line found for the attribute of a node, through families, tasks with and without endtask, and
///        aliases.
///
BOOST_AUTO_TEST_CASE(test_line_of_permissions) {
    ECF_NAME_THIS_TEST();

    const std::string text = "suite s\n"                 // 1
                             "  permissions [1]\n"       // 2
                             "  task t\n"                // 3
                             "    permissions [2]\n"     // 4
                             "  endtask\n"               // 5
                             "  family f\n"              // 6
                             "    permissions [3]\n"     // 7
                             "    task u\n"              // 8
                             "      permissions [4]\n"   // 9
                             "      alias a\n"           // 10
                             "        permissions [5]\n" // 11
                             "      endalias\n"          // 12
                             "    task v\n"              // 13
                             "  endfamily\n"             // 14
                             "  family g\n"              // 15
                             "    permissions [6]\n"     // 16
                             "  endfamily\n"             // 17
                             "endsuite\n";               // 18
    const std::vector<std::pair<std::string, int>> expected{
        {"/s", 2}, {"/s/t", 4}, {"/s/f", 7}, {"/s/f/u", 9}, {"/s/f/u/a", 11}, {"/s/g", 16}, {"/s/f/v", 0}, {"/x", 0}};
    for (const auto& [path, line] : expected) {
        std::istringstream in(text);
        BOOST_CHECK_MESSAGE(ecf::line_of_permissions(in, path) == line, path);
    }
}

///
/// @brief Verifies the line found for the attributes of successive aliases of a task, which a checkpoint writes as
///        siblings closed by a single endalias, and of the task following them.
///
BOOST_AUTO_TEST_CASE(test_line_of_permissions_successive_aliases) {
    ECF_NAME_THIS_TEST();

    const std::string text = "suite s\n"               // 1
                             "  task t\n"              // 2
                             "    alias alias0\n"      // 3
                             "      permissions [1]\n" // 4
                             "    alias alias1\n"      // 5
                             "      permissions [2]\n" // 6
                             "    endalias\n"          // 7
                             "  task u\n"              // 8
                             "    permissions [3]\n"   // 9
                             "endsuite\n";             // 10
    const std::vector<std::pair<std::string, int>> expected{
        {"/s/t/alias0", 4}, {"/s/t/alias1", 6}, {"/s/u", 9}, {"/s/t/alias0/alias1", 0}};
    for (const auto& [path, line] : expected) {
        std::istringstream in(text);
        BOOST_CHECK_MESSAGE(ecf::line_of_permissions(in, path) == line, path);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
