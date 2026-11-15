// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "PermissionsServer.hpp"
#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/base/Client.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/user/MoveCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::test::Credentials;
using ecf::test::PermissionsServer;

///
/// The roles of the commands that load, replace, plug, move, delete and begin nodes (D4.5, D4.7, D4.11, D4.14, D4.15,
/// R11.4, R4.5, R11.9), in Permissions mode.
///

namespace {

const Credentials admin{"admin", "admin-secret"};

///
/// @brief Returns the credentials of a user named after the roles it holds at server level, e.g. "rwo".
///
Credentials user(const std::string& roles) {
    return Credentials{"user" + roles, "secret-" + roles};
}

const std::vector<std::string> role_sets{"r", "rw", "ro", "rwp", "rop", "rwo", "rwop", "rwxp"};

std::vector<Credentials> all_users() {
    std::vector<Credentials> users;
    for (const auto& roles : role_sets) {
        users.push_back(user(roles));
    }
    return users;
}

std::string server_attribute() {
    std::string attribute = R"([{"user": "admin", "permissions": "rwxlops"})";
    for (const auto& roles : role_sets) {
        attribute += R"(, {"user": ")" + user(roles).user + R"(", "permissions": ")" + roles + R"("})";
    }
    return attribute + "]";
}

///
/// @brief Builds definitions holding the given suites, each with family f and task t.
///
/// @param[in] suites The suites, each with the command-line form of its attribute, or "" for none
///
defs_ptr make_defs(const std::map<std::string, std::string>& suites) {
    auto defs = Defs::create();
    for (const auto& [name, attribute] : suites) {
        auto suite = defs->add_suite(name);
        suite->add_family("f")->add_task("t");
        if (!attribute.empty()) {
            suite->add_permissions(ecf::Permissions::parse_cli(attribute));
        }
    }
    return defs;
}

///
/// @brief Joins lines, one per line.
///
std::string to_text(const std::vector<std::string>& lines) {
    std::string text;
    for (const auto& line : lines) {
        text += line + "\n";
    }
    return text;
}

///
/// @brief Returns the content of a file.
///
std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

///
/// @brief Gives the server its initial suites: s (no attribute), guarded (an attribute listing every user with the
///        roles it holds at server level), secret (the administrator only), each holding family f with task t, and
///        landing (no attribute, empty), which receives plugged nodes.
///
void reset(const PermissionsServer& server) {
    auto client = server.administrator();
    (void)client->delete_all(true);
    std::string everyone = "admin:rwxlop";
    for (const auto& roles : role_sets) {
        everyone += "," + user(roles).user + ":" + roles;
    }
    auto defs = make_defs({{"s", ""}, {"guarded", everyone}, {"secret", "admin:rwxlop"}});
    defs->add_suite("landing");
    BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
}

///
/// @brief Starts a server with the server attribute listing every user, and its initial suites.
///
std::unique_ptr<PermissionsServer> start(bool http = false) {
    auto server = std::make_unique<PermissionsServer>(admin,
                                                      all_users(),
                                                      server_attribute(),
                                                      std::chrono::seconds(30),
                                                      std::nullopt,
                                                      std::nullopt,
                                                      false,
                                                      http ? R"("http": true)" : "");
    reset(*server);
    return server;
}

///
/// @brief Checks which users a request is denied to, each sending it to the server in its initial state.
///
/// @tparam Send A callable `int(ClientInvoker&)` sending the request
/// @param[in] what The request, for the messages
/// @param[in] allowed The role sets that are allowed, and for which the request succeeds; all others are denied
/// @param[in] send Sends the request with the given client
///
template <typename Send>
void check(const std::string& what, const std::vector<std::string>& allowed, Send send) {
    for (const bool http : {false, true}) {
        auto server = start(http);
        for (const auto& roles : role_sets) {
            BOOST_TEST_CONTEXT(what << " by " << user(roles).user << (http ? " over HTTP" : " over TCP")) {
                reset(*server);
                auto client         = server->client(user(roles));
                const int status    = send(*client);
                const bool expected = std::find(allowed.begin(), allowed.end(), roles) == allowed.end();
                BOOST_CHECK_EQUAL(client->server_reply().permission_denied(), expected);
                BOOST_CHECK_MESSAGE((status != 0) == expected, client->errorMsg());
            }
        }
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_PermissionsLifecycle)

///
/// @brief Verifies the roles of --load: o at server level, p for attributes carried or overwritten, w for server
///        variables (D4.5, R4.5, R11.4).
///
BOOST_AUTO_TEST_CASE(test_load) {
    ECF_NAME_THIS_TEST();

    check("load a new suite", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.load(make_defs({{"new", ""}}));
    });
    check("load a new suite carrying an attribute", {"rop", "rwop"}, [](ClientInvoker& c) {
        return c.load(make_defs({{"new", "admin:rwxlop"}}));
    });
    check("load --force over a suite without attribute", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.load(make_defs({{"s", ""}}), true);
    });
    check("load --force over a suite with an attribute", {"rop", "rwop"}, [](ClientInvoker& c) {
        return c.load(make_defs({{"guarded", ""}}), true);
    });
    check("load with server variables", {"rwo", "rwop"}, [](ClientInvoker& c) {
        auto defs = make_defs({{"new", ""}});
        defs->server_state().add_or_update_user_variables("FOO", "bar");
        return c.load(defs);
    });
}

///
/// @brief Verifies the roles of --replace: w at the node, o for a suite, p for attributes (D4.5, D4.10, D4.14, R4.5).
///
BOOST_AUTO_TEST_CASE(test_replace) {
    ECF_NAME_THIS_TEST();

    check("replace a family", {"rw", "rwp", "rwo", "rwop", "rwxp"}, [](ClientInvoker& c) {
        return c.replace_1("/s/f", make_defs({{"s", ""}}));
    });
    check("replace a family with one carrying an attribute", {"rwp", "rwop", "rwxp"}, [](ClientInvoker& c) {
        auto defs = make_defs({{"s", ""}});
        defs->findAbsNode("/s/f")->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
        return c.replace_1("/s/f", defs);
    });
    check("replace a suite carrying an attribute with one carrying none", {"rop", "rwop"}, [](ClientInvoker& c) {
        return c.replace_1("/guarded", make_defs({{"guarded", ""}}));
    });
    check("replace a suite", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.replace_1("/s", make_defs({{"s", ""}}));
    });
    check("replace creating a family", {"rw", "rwp", "rwo", "rwop", "rwxp"}, [](ClientInvoker& c) {
        auto defs = make_defs({{"s", ""}});
        defs->findSuite("s")->add_family("g")->add_task("t");
        return c.replace_1("/s/g", defs, true, false);
    });
    check("replace creating a suite", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.replace_1("/new/f", make_defs({{"new", ""}}), true, false);
    });
    check("replace creating a suite carrying an attribute", {"rop", "rwop"}, [](ClientInvoker& c) {
        return c.replace_1("/new/f", make_defs({{"new", "admin:rwxlop"}}), true, false);
    });
    check("replace creating a family carrying an attribute", {"rwp", "rwop", "rwxp"}, [](ClientInvoker& c) {
        auto defs   = make_defs({{"s", ""}});
        auto family = defs->findSuite("s")->add_family("g");
        family->add_task("t");
        family->add_permissions(ecf::Permissions::parse_cli("admin:rwxlop"));
        return c.replace_1("/s/g/t", defs, true, false); // creates g, with its attribute
    });
    check("replace creating a suite, by a relative path", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.replace_1("xs/f", make_defs({{"xs", ""}}), true, false);
    });
    check("replace in a suite open to the administrator only", {}, [](ClientInvoker& c) {
        return c.replace_1("/secret/f", make_defs({{"secret", ""}}));
    });
}

///
/// @brief Verifies the roles of --delete and --begin: w for a node, o for a suite, every path checked (D4.7, R5.12).
///
BOOST_AUTO_TEST_CASE(test_delete_and_begin) {
    ECF_NAME_THIS_TEST();

    check("delete a family", {"rw", "rwp", "rwo", "rwop", "rwxp"}, [](ClientInvoker& c) {
        return c.delete_node("/s/f", true);
    });
    check("delete a family and one of a suite open to the administrator only", {}, [](ClientInvoker& c) {
        return c.delete_nodes({"/s/f", "/secret/f"}, true);
    });
    check("delete a suite", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) { return c.delete_node("/s", true); });
    check("delete a suite named /s/", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.delete_node("/s/", true);
    });
    check("delete a suite named //s", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) {
        return c.delete_node("//s", true);
    });
    check("delete all", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) { return c.delete_all(true); });
    check("begin a suite", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) { return c.begin("s"); });
    check("begin all suites", {"ro", "rop", "rwo", "rwop"}, [](ClientInvoker& c) { return c.begin_all_suites(); });
}

///
/// @brief Verifies the roles of a local --plug: w and p at the source and the destination (D4.11, R4.5).
///
BOOST_AUTO_TEST_CASE(test_local_plug) {
    ECF_NAME_THIS_TEST();

    check("plug a family into a suite", {"rwp", "rwop", "rwxp"}, [](ClientInvoker& c) {
        return c.plug("/s/f", "/landing");
    });
    check("plug a family into a suite open to the administrator only", {}, [](ClientInvoker& c) {
        return c.plug("/s/f", "/secret");
    });
    check("plug a family of a suite open to the administrator only", {}, [](ClientInvoker& c) {
        return c.plug("/secret/f", "/s");
    });
}

///
/// @brief Verifies that a plug to this server, named by host and port, is authorised as a local plug, at both ends.
///
BOOST_AUTO_TEST_CASE(test_plug_to_this_server_by_host_and_port) {
    ECF_NAME_THIS_TEST();

    for (const auto& [destination, allowed] : std::vector<std::pair<std::string, std::vector<std::string>>>{
             {"/landing", {"rwp", "rwop", "rwxp"}}, {"/secret", {}}}) {
        auto server       = start();
        const auto target = "localhost:" + server->port() + destination;
        for (const auto& roles : role_sets) {
            BOOST_TEST_CONTEXT("plug to " << target << " by " << user(roles).user) {
                reset(*server);
                auto client         = server->client(user(roles));
                const int status    = client->plug("/s/f", target);
                const bool expected = std::find(allowed.begin(), allowed.end(), roles) == allowed.end();
                BOOST_CHECK_EQUAL(client->server_reply().permission_denied(), expected);
                BOOST_CHECK_MESSAGE((status != 0) == expected, client->errorMsg());
            }
        }
    }
}

///
/// @brief Verifies that a move sent directly by a client is authorised as the receiving side of a plug: w and p at
///        the destination, o (and p for attributes) for a whole suite (D4.11, D4.15, R4.5).
///
BOOST_AUTO_TEST_CASE(test_direct_move) {
    ECF_NAME_THIS_TEST();

    auto payload      = make_defs({{"carried", ""}, {"guarded_suite", "admin:rwxlop"}});
    const auto origin = std::make_pair(std::string("elsewhere"), std::string("3141"));

    // Sends the move as a client would, with the credentials of the user (a custom user)
    const auto send = [&](const PermissionsServer& server,
                          const Credentials& credentials,
                          Node* node,
                          const std::string& destination) {
        auto move = std::make_shared<MoveCmd>(origin, node, destination);
        move->setup_user_authentification(ecf::Identity::make_custom_user(
            credentials.user, PasswordEncryption::encrypt(credentials.password, credentials.user)));
        ServerReply reply;
        boost::asio::io_context io;
        Client client(io, move, std::string(ecf::string_constants::localhost), server.port());
        io.run();
        client.handle_server_response(reply, false);
        return std::make_tuple(reply.permission_denied(), reply.client_request_failed(), reply.error_msg());
    };

    for (const auto& [what, node, destination, allowed] :
         std::vector<std::tuple<std::string, Node*, std::string, std::vector<std::string>>>{
             {"move a family under a suite",
              payload->findAbsNode("/carried/f").get(),
              "/landing",
              {"rwp", "rwop", "rwxp"}},
             {"move a whole suite carrying an attribute",
              payload->findSuite("guarded_suite").get(),
              "",
              {"rop", "rwop"}},
         }) {
        auto server = start();
        for (const auto& roles : role_sets) {
            BOOST_TEST_CONTEXT(what << " by " << user(roles).user) {
                reset(*server);
                const auto [denied, failed, error] = send(*server, user(roles), node, destination);
                const bool expected                = std::find(allowed.begin(), allowed.end(), roles) == allowed.end();
                BOOST_CHECK_EQUAL(denied, expected);
                BOOST_CHECK_MESSAGE(failed == expected, error);
            }
        }
    }
}

///
/// @brief Verifies that a plug to another server is authorised there, under the identity of the user, and that a
///        denial reaches the user as a denial (R11.9, D4.11, D4.15).
///
BOOST_AUTO_TEST_CASE(test_remote_plug) {
    ECF_NAME_THIS_TEST();

    const auto mover = user("rwxp");
    struct Case
    {
        std::string what;
        std::string receiver_roles; ///< The roles of the user at server level on the receiving server
        std::string source;
        std::string destination; ///< The path on the receiving server ("" for a whole suite)
        bool denied;
        std::string archive{}; ///< A container archived before the plug, if any
        bool http{false};      ///< True when the sending server uses HTTP
    };
    const std::vector<Case> cases{
        {"a family, the user holding r there", "r", "/s/f", "/s", true},
        {"a family, the user holding rwp there", "rwp", "/s/f", "/s", false},
        {"a whole suite, the user holding rwp there", "rwp", "/s", "", true},
        {"a whole suite, the user holding rop there", "rop", "/s", "", false},
        {"a family, the user holding rw there (no p)", "rw", "/s/f", "/s", true},
        {"a family, the user holding rp there (no w)", "rp", "/s/f", "/s", true},
        {"a whole suite carrying an attribute, the user holding ro there", "ro", "/guarded", "", true},
        {"a whole suite carrying an attribute, the user holding rop there", "rop", "/guarded", "", false},
        {"an archived family, the user holding r there", "r", "/s/f", "/s", true, "/s/f"},
        {"a whole suite holding an archived family, the user holding r there", "r", "/s", "", true, "/s/f"},
        {"a whole suite holding an archived family, the user holding rop there", "rop", "/s", "", false, "/s/f"},
        {"a family from an HTTP server, the user holding r there", "r", "/s/f", "/s", true, "", true},
        {"a family from an HTTP server, the user holding rwp there", "rwp", "/s/f", "/s", false, "", true},
    };
    for (const auto& c : cases) {
        BOOST_TEST_CONTEXT(c.what) {
            auto sender = start(c.http);
            // The archive, and what a denied plug must leave unchanged: the archive file and the edit history of the
            // server, where any edit would add an entry
            const auto archive_file = sender->home() / ecf::Host{}.prefix_host_and_port(sender->port(), ":s:f.check");
            std::string archive_before, history_before;
            if (!c.archive.empty()) {
                auto archiver = sender->administrator();
                BOOST_REQUIRE_MESSAGE(archiver->archive(c.archive) == 0, archiver->errorMsg());
                BOOST_REQUIRE_MESSAGE(fs::exists(archive_file), archive_file);
                archive_before = read_file(archive_file);
                BOOST_REQUIRE_MESSAGE(archiver->edit_history("/") == 0, archiver->errorMsg());
                history_before = to_text(archiver->server_reply().get_string_vec());
            }
            PermissionsServer receiver(admin,
                                       {mover},
                                       R"([{"user": "admin", "permissions": "rwxlops"}, {"user": ")" + mover.user +
                                           R"(", "permissions": ")" + c.receiver_roles + R"("}])");
            auto setup = receiver.administrator();
            auto empty = Defs::create();
            empty->add_suite("s2"); // receives the family f
            BOOST_REQUIRE_MESSAGE(setup->load(empty) == 0, setup->errorMsg());
            const auto destination = "localhost:" + receiver.port() + (c.destination.empty() ? "" : "/s2");

            auto client      = sender->client(mover);
            const int status = client->plug(c.source, destination);
            BOOST_CHECK_EQUAL(client->server_reply().permission_denied(), c.denied);
            BOOST_CHECK_MESSAGE((status != 0) == c.denied, client->errorMsg());

            // A denial by the receiving server names it, the request and the requester, in the reply and, once, in
            // the log of the sending server
            if (c.denied) {
                const auto receiver_named = "the receiving server localhost:" + receiver.port() + " denied the move";
                BOOST_CHECK_MESSAGE(client->errorMsg().find(receiver_named) != std::string::npos, client->errorMsg());
                BOOST_CHECK_MESSAGE(client->errorMsg().find("[--plug") != std::string::npos, client->errorMsg());
                BOOST_CHECK_MESSAGE(client->errorMsg().find(":" + mover.user + "@") != std::string::npos,
                                    client->errorMsg());
                std::ifstream log(sender->home() / "server.log");
                std::size_t records = 0;
                for (std::string line; std::getline(log, line);) {
                    records += line.find(receiver_named) != std::string::npos ? 1 : 0;
                }
                BOOST_CHECK_EQUAL(records, 1U);
            }

            // On a denial, the source stays on the sending server
            auto check = sender->administrator();
            BOOST_REQUIRE_MESSAGE(check->getDefs() == 0, check->errorMsg());
            BOOST_CHECK_EQUAL(check->defs()->findAbsNode(c.source) != nullptr, c.denied);
            if (c.archive.empty()) {
                continue;
            }
            if (c.denied) {
                // The archive file and the edit history of the server are unchanged
                BOOST_CHECK(fs::exists(archive_file) && read_file(archive_file) == archive_before);
                BOOST_REQUIRE_MESSAGE(check->edit_history("/") == 0, check->errorMsg());
                BOOST_CHECK_EQUAL(to_text(check->server_reply().get_string_vec()), history_before);

                // The archived container is still archived, without children, and its archive restores it
                BOOST_REQUIRE_MESSAGE(check->getDefs() == 0, check->errorMsg());
                const auto archived = check->defs()->findAbsNode(c.archive);
                BOOST_REQUIRE(archived);
                BOOST_CHECK(archived->get_flag().is_set(ecf::Flag::ARCHIVED));
                std::vector<node_ptr> children;
                archived->immediateChildren(children);
                BOOST_CHECK(children.empty());
                BOOST_REQUIRE_MESSAGE(check->restore(c.archive) == 0, check->errorMsg());
                BOOST_REQUIRE_MESSAGE(check->getDefs() == 0, check->errorMsg());
                BOOST_CHECK(check->defs()->findAbsNode(c.archive + "/t") != nullptr);
            }
            else {
                // The receiving server holds the archived container restored, with its own ECF_HOME (none was added),
                // and the sending server no longer holds the archive
                BOOST_REQUIRE_MESSAGE(setup->getDefs() == 0, setup->errorMsg());
                BOOST_CHECK(setup->defs()->findAbsNode(c.archive + "/t") != nullptr);
                BOOST_CHECK(setup->defs()->findAbsNode(c.source)->findVariable("ECF_HOME").empty());
                BOOST_CHECK(!fs::exists(archive_file));
            }
        }
    }
}

///
/// @brief Verifies the targets of the lifecycle commands in white-list mode (D13.4): the server for o, the nodes for w
///        and p, the destination of a plug included; a denied request changes nothing.
///
BOOST_AUTO_TEST_CASE(test_lifecycle_in_whitelist_mode) {
    ECF_NAME_THIS_TEST();

    const Credentials writer{"writer", "writer-secret"}; // write access everywhere
    const Credentials reader{"reader", "reader-secret"}; // read access everywhere
    const Credentials scoped{"scoped", "scoped-secret"}; // write access to the suite s only
    PermissionsServer server(admin, {writer, reader, scoped}, "");
    std::ofstream(server.home() / ecf::Host{}.prefix_host_and_port(server.port(), "ecf.lists"))
        << "4.5.0\nadmin\nwriter\n-reader\nscoped /s\n";
    BOOST_REQUIRE_EQUAL(server.administrator()->reloadwsfile(), 0);

    const auto reset_suites = [&] {
        auto client = server.administrator();
        (void)client->delete_all(true);
        auto defs = make_defs({{"s", ""}});
        defs->add_suite("other")->add_family("g")->add_task("t");
        defs->add_suite("landing");
        BOOST_REQUIRE_MESSAGE(client->load(defs) == 0, client->errorMsg());
    };
    const auto exists = [&](const std::string& path) {
        auto client = server.administrator();
        BOOST_REQUIRE_MESSAGE(client->getDefs() == 0, client->errorMsg());
        return client->defs()->findAbsNode(path) != nullptr;
    };

    struct Case
    {
        std::string what;
        std::function<int(ClientInvoker&)> send;
        std::vector<std::string> allowed; ///< The users allowed
        std::string unchanged;            ///< A node that a denied request leaves in place, if any
    };
    const std::vector<Case> cases{
        {"load a new suite", [](ClientInvoker& c) { return c.load(make_defs({{"new", ""}})); }, {"writer"}, ""},
        {"begin a suite", [](ClientInvoker& c) { return c.begin("s"); }, {"writer"}, ""},
        {"delete a family", [](ClientInvoker& c) { return c.delete_node("/s/f", true); }, {"writer", "scoped"}, "/s/f"},
        {"delete a suite", [](ClientInvoker& c) { return c.delete_node("/s", true); }, {"writer"}, "/s"},
        {"replace a family",
         [](ClientInvoker& c) { return c.replace_1("/s/f", make_defs({{"s", ""}})); },
         {"writer", "scoped"},
         ""},
        {"plug a family out of the scope",
         [](ClientInvoker& c) { return c.plug("/s/f", "/landing"); },
         {"writer"},
         "/s/f"},
        {"plug a family into the scope",
         [](ClientInvoker& c) { return c.plug("/other/g", "/s"); },
         {"writer"},
         "/other/g"},
    };
    for (const auto& c : cases) {
        for (const auto& user : {writer, reader, scoped}) {
            BOOST_TEST_CONTEXT(c.what << " by " << user.user) {
                reset_suites();
                auto client         = server.client(user);
                const int status    = c.send(*client);
                const bool expected = std::find(c.allowed.begin(), c.allowed.end(), user.user) == c.allowed.end();
                BOOST_CHECK_EQUAL(client->server_reply().permission_denied(), expected);
                BOOST_CHECK_MESSAGE((status != 0) == expected, client->errorMsg());
                if (expected && !c.unchanged.empty()) {
                    BOOST_CHECK(exists(c.unchanged));
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
