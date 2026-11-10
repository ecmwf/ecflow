// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iterator>
#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include "PrivilegedEnvironment.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf::test::ownership;

BOOST_AUTO_TEST_SUITE(S_Ownership)

BOOST_AUTO_TEST_SUITE(T_Plug)

namespace {

///
/// Definitions with suite s (Owner suite_owner, if any), family f (Owner f_owner) and task t
///
defs_ptr make_defs(const std::string& suite, const std::string& suite_owner, const std::string& f_owner) {
    defs_ptr defs = Defs::create();
    suite_ptr s   = defs->add_suite(suite);
    if (!suite_owner.empty()) {
        s->addOwner(ecf::OwnerAttr::create(suite_owner));
    }
    family_ptr f = s->add_family("f");
    f->addOwner(ecf::OwnerAttr::create(f_owner));
    f->add_task("t");
    return defs;
}

/// The definitions of a server, as text
std::string defs_of(const LaunchedServer& server) {
    ClientInvoker client;
    server.configure(client, alice);
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    return ecf::as_string(*client.defs(), PrintStyle::DEFS);
}

/// The archive files of the families in the home of a server, by name, with their content
std::map<std::string, std::string> archives_of(const LaunchedServer& server) {
    std::map<std::string, std::string> archives;
    for (const auto& entry : fs::directory_iterator(server.home())) {
        const auto name = entry.path().filename().string();
        if (name.find(":f.check") != std::string::npos) {
            std::ifstream input(entry.path(), std::ios::binary);
            archives[name] = std::string{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        }
    }
    return archives;
}

/// The content of an archive, without the flag 'restored' (a refused remote plug restores the archived nodes, then
/// archives them again, which sets it)
std::string without_restored_flag(std::string content) {
    for (const std::string flag : {",restored", "restored,", " # flag:restored"}) {
        for (auto at = content.find(flag); at != std::string::npos; at = content.find(flag)) {
            content.erase(at, flag.size());
        }
    }
    return content;
}

/// Checks that the archive files hold the same definitions, reporting the first that differs
void check_archives_unchanged(const std::map<std::string, std::string>& after,
                              const std::map<std::string, std::string>& before) {
    BOOST_CHECK_EQUAL(after.size(), before.size());
    for (const auto& [name, content] : before) {
        const auto found = after.find(name);
        BOOST_CHECK_MESSAGE(found != after.end(), "Archive " << name << " removed");
        if (found != after.end()) {
            BOOST_CHECK_MESSAGE(without_restored_flag(found->second) == without_restored_flag(content),
                                "Archive " << name << " changed; before:\n"
                                           << content << "\nafter:\n"
                                           << found->second);
        }
    }
}

/// The destination of a plug moving a whole suite to another server
std::string destination(const LaunchedServer& server) {
    return "//localhost:" + server.port();
}

/// Loads definitions as the given user, and archives their family
void load_and_archive(const LaunchedServer& server, const std::string& user, const defs_ptr& defs) {
    ClientInvoker client;
    server.configure(client, user);
    BOOST_REQUIRE_MESSAGE(client.load(defs) == 0, client.errorMsg());
    const auto family = "/" + defs->suites().front()->name() + "/f";
    BOOST_REQUIRE_MESSAGE(client.archive(family) == 0, client.errorMsg());
}

} // namespace

BOOST_AUTO_TEST_CASE(test_remote_plug_between_servers) {
    ECF_NAME_THIS_TEST();

    LaunchedServer source{Launch{}};
    LaunchedServer target{Launch{}};
    // A destination run by bob, where bob cannot be an owner
    Launch run_by_bob;
    run_by_bob.account = bob;
    run_by_bob.owner   = charlie;
    LaunchedServer bobs{run_by_bob};
    source.require_ready();
    target.require_ready();
    bobs.require_ready();

    // In r and q, only the archived family holds an Owner, which alone decides the refusals below
    load_and_archive(source, alice, make_defs("p", alice, alice + ":" + owners_group));
    load_and_archive(source, alice, make_defs("r", "", alice));
    load_and_archive(source, bob, make_defs("q", "", bob));

    const auto source_defs     = defs_of(source);
    const auto source_archives = archives_of(source);
    BOOST_REQUIRE_EQUAL(source_archives.size(), 3U);

    // refused at the source: only the current owner can move an archived Owner away
    ClientInvoker client;
    source.configure(client, bob);
    BOOST_CHECK_EQUAL(client.plug("/r", destination(target)), 1);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("only the current owner 'alice' can remove or replace its Owner, "
                                               "not 'bob'") != std::string::npos,
                        client.errorMsg());
    BOOST_CHECK_EQUAL(defs_of(source), source_defs);
    check_archives_unchanged(archives_of(source), source_archives);
    BOOST_CHECK(defs_of(target).find("suite r") == std::string::npos);

    // refused at the destination, because of the archived Owner: the source is left unchanged
    BOOST_CHECK_EQUAL(client.plug("/q", destination(bobs)), 1);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("The owner '" + bob + "' is the server account") != std::string::npos,
                        client.errorMsg());
    BOOST_CHECK_EQUAL(defs_of(source), source_defs);
    check_archives_unchanged(archives_of(source), source_archives);
    BOOST_CHECK(defs_of(bobs).find("suite q") == std::string::npos);

    // accepted: the subtree moves with its Owners, the archived family restored
    source.configure(client, alice);
    BOOST_REQUIRE_MESSAGE(client.plug("/p", destination(target)) == 0, client.errorMsg());
    BOOST_CHECK(defs_of(source).find("suite p") == std::string::npos);
    target.configure(client, bob);
    BOOST_REQUIRE_MESSAGE(client.ownership("read", "/p/f/t") == 0, client.errorMsg());
    BOOST_CHECK_EQUAL(client.get_string(), "owner=none\neffective=" + alice + ":" + owners_group + "\norigin=/p/f\n");
    BOOST_REQUIRE_MESSAGE(client.ownership("read", "/p") == 0, client.errorMsg());
    BOOST_CHECK_EQUAL(client.get_string(), "owner=" + alice + "\neffective=" + alice + "\norigin=/p\n");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
