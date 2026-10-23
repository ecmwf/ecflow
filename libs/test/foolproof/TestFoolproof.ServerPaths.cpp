// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <list>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Verifies runtime file access and exported paths with separate startup, home and admin directories.
///
/// @param[in] from_environment Whether the six filenames are selected through environment overrides
/// @param[in] form Relative, absolute or space-containing path form
///
void check_server_runtime_paths(bool from_environment, const std::string& form) {
    using namespace ecf::test::scaffold;

    auto root        = MakeDirectory{}.create();
    const auto home  = root.path() / "server home";
    const auto admin = root.path() / "admin files";
    Directory startup(root.path() / "startup");
    fs::create_directories(startup.path());
    fs::create_directories(home);
    fs::create_directories(admin);

    std::list<WithoutTestEnvironmentVariable> cleared;
    for (const auto* name : {"ECF_HOME",
                             "ECF_PORT",
                             "ECF_LOG",
                             "ECF_CHECK",
                             "ECF_CHECKOLD",
                             "ECF_PASSWD",
                             "ECF_CUSTOM_PASSWD",
                             "ECF_LISTS",
                             "ECF_CHECKINTERVAL",
                             "ECF_PRUNE_NODE_LOG",
                             "ECF_TASK_THRESHOLD",
                             "ECF_SSL",
                             "ECF_DEBUG_SERVER",
                             "ECF_HOST",
                             "ECF_USER",
                             "ECF_PASS",
                             "ECF_PASSWORD"}) {
        cleared.emplace_back(name);
    }

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const User allowed{"path_reader", "selected_secret", "rw"};
    const User excluded{"excluded_reader", "excluded_secret", "rw"};
    const User regular{ecf::get_login_name(), "regular_secret", "rw"};

    std::list<WithTestFile> files;
    std::list<WithTestEnvironmentVariable> overrides;
    std::vector<std::pair<std::string, std::string>> expected;
    std::vector<fs::path> destinations;
    std::string config = "ECF_HOME = " + home.string() + "\nECF_CHECKMODE = CHECK_NEVER\n";

    for (const auto& [name, basename] :
         std::vector<std::pair<std::string, std::string>>{{"ECF_LOG", "server.log"},
                                                          {"ECF_CHECK", "primary.check"},
                                                          {"ECF_CHECKOLD", "backup.check"},
                                                          {"ECF_PASSWD", "regular.passwd"},
                                                          {"ECF_CUSTOM_PASSWD", "custom.passwd"},
                                                          {"ECF_LISTS", "users.lists"}}) {
        std::string input = "runtime/" + basename;
        if (form == "absolute") {
            input = (admin / basename).string();
        }
        else if (form == "spaces") {
            input = "runtime with spaces/selected " + basename;
        }

        const auto destination = home / input;
        fs::create_directories(destination.parent_path());
        destinations.push_back(destination);
        const bool security = name == "ECF_PASSWD" || name == "ECF_CUSTOM_PASSWD" || name == "ECF_LISTS";
        expected.emplace_back(name, security ? input : destination.string());

        if (security) {
            const auto content = name == "ECF_LISTS"    ? WhitelistFile{allowed, regular}.data()
                                 : name == "ECF_PASSWD" ? PasswordsFile{host, port, regular}.data()
                                                        : PasswordsFile{host, port, allowed, excluded}.data();
            files.emplace_back(NamedTestFile{destination.string()}, content);
            if (form != "absolute") {
                const auto decoy = startup.path() / input;
                fs::create_directories(decoy.parent_path());
                // The readiness client also reads ECF_PASSWD from its startup directory.
                // Keep that decoy parseable while giving it different credentials.
                const auto decoy_content =
                    name == "ECF_PASSWD" ? PasswordsFile{host, port, User{"decoy_user", "decoy_secret", "rw"}}.data()
                                         : std::string("malformed startup-side security file\n");
                files.emplace_back(NamedTestFile{decoy.string()}, decoy_content);
            }
        }

        if (from_environment) {
            overrides.emplace_back(name, input);
        }
        else {
            config += name + " = " + input + "\n";
        }
    }

    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

    auto server = MakeServer{}.with(host).with(port).with(startup).launch();
    BOOST_REQUIRE_MESSAGE(server.ok(), (server.ok() ? "" : server.reason()));

    ClientInvoker client;
    client.set_throw_on_error(false);
    client.set_host_port(host.value(), std::to_string(port.value()));
    client.set_connection_attempts(1);
    client.set_connect_timeout(std::chrono::seconds(5));
    client.set_user_name(allowed.username);
    client.set_password(PasswordEncryption::encrypt(allowed.password, allowed.username));

    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());

    for (const auto& [name, value] : expected) {
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(name), value);
    }
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_HOME"), home.string());
    BOOST_CHECK(fs::is_regular_file(destinations[0]));

    // Explicit saves exercise both selected checkpoint destinations without waiting for a timer.
    BOOST_REQUIRE_MESSAGE(client.checkPtDefs() == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.checkPtDefs() == 0, client.errorMsg());
    BOOST_CHECK(fs::is_regular_file(destinations[1]));
    BOOST_CHECK(fs::is_regular_file(destinations[2]));

    for (size_t i = 0; i < 3; ++i) {
        if (form != "absolute") {
            BOOST_CHECK(!fs::exists(startup.path() / fs::relative(destinations[i], home)));
        }
    }

    client.set_password(PasswordEncryption::encrypt("incorrect_secret", allowed.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());

    client.set_user_name(excluded.username);
    client.set_password(PasswordEncryption::encrypt(excluded.password, excluded.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("white") != std::string::npos ||
                            client.errorMsg().find("Authorisation") != std::string::npos,
                        client.errorMsg());

    client.set_user_name("");
    client.set_password(PasswordEncryption::encrypt(regular.password, regular.username));
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());

    client.set_password(PasswordEncryption::encrypt("incorrect_regular_secret", regular.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerPaths)

///
/// @brief Verifies actual file access and exported definition paths for config-selected filenames.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_from_config) {
    ECF_NAME_THIS_TEST();

    for (const auto& form : {"relative", "absolute", "spaces"}) {
        BOOST_TEST_CONTEXT("Path form: " << form) {
            check_server_runtime_paths(false, form);
        }
    }
}

///
/// @brief Verifies actual file access and exported definition paths for environment-selected filenames.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_from_environment) {
    ECF_NAME_THIS_TEST();

    for (const auto& form : {"relative", "absolute", "spaces"}) {
        BOOST_TEST_CONTEXT("Path form: " << form) {
            check_server_runtime_paths(true, form);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
