// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <chrono>
#include <fstream>
#include <list>
#include <string>
#include <thread>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/User.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Rewrites an owned fixture and closes it before sending a reload request.
///
/// @param[in] path Owned configuration or security-file path
/// @param[in] content Complete replacement contents
///
void rewrite_reload_fixture(const fs::path& path, const std::string& content) {
    std::ofstream output(path, std::ios::trunc);
    BOOST_REQUIRE_MESSAGE(output.is_open(), "Unable to rewrite fixture: " << path);
    output << content;
    output.close();
    BOOST_REQUIRE_MESSAGE(output.good(), "Unable to finish rewriting fixture: " << path);
}

///
/// @brief Selects an identity and verifies its effective read or write access.
///
/// @param[in,out] client Client targeting the isolated server
/// @param[in] user Identity and plain-text fixture password
/// @param[in] custom Whether the identity uses the custom-password database
/// @param[in] allowed Required access outcome
/// @param[in] write Whether to request a checkpoint-mode change rather than reading the definition
/// @param[in] cause Required diagnostic fragment when access is refused
///
void check_reload_access(ClientInvoker& client,
                         const ecf::test::scaffold::User& user,
                         bool custom,
                         bool allowed,
                         bool write               = false,
                         const std::string& cause = {}) {
    client.set_user_name(custom ? user.username : "");
    client.set_password(PasswordEncryption::encrypt(user.password, user.username));
    const int status = write ? client.checkPtDefs(ecf::CheckPt::NEVER) : client.getDefs();
    BOOST_CHECK_MESSAGE((status == 0) == allowed,
                        "Unexpected " << (write ? "write" : "read") << " access for " << user.username << ": "
                                      << client.errorMsg());
    if (!allowed) {
        BOOST_CHECK_MESSAGE(client.errorMsg().find(cause) != std::string::npos, client.errorMsg());
    }
}

///
/// @brief Checks the selected reload command and the resulting credentials and permissions through live requests.
///
/// @param[in] index Regular-password, custom-password or whitelist index
/// @param[in] action Config redirection, successful update, removal or malformed replacement scenario
/// @param[in] from_environment Whether original security paths are selected by environment overrides
///
void check_security_reload(size_t index, const std::string& action, bool from_environment) {
    using namespace ecf::test::scaffold;

    auto startup    = MakeDirectory{}.create();
    const auto home = startup.path() / "server home";
    fs::create_directories(home);

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
                             "ECF_USER",
                             "ECF_HOST",
                             "ECF_PASS",
                             "ECF_PASSWORD"}) {
        cleared.emplace_back(name);
    }

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const User regular{ecf::get_login_name(), "original_regular", "rw"};
    const User rotated_regular{regular.username, "rotated_regular", "rw"};
    const User controller{"reload_controller", "controller_secret", "rw"};
    const User probe{"reload_probe", "original_probe", "rw"};
    const User rotated_probe{probe.username, "rotated_probe", "rw"};
    const User read_only_probe{probe.username, probe.password, "r"};
    const User removed{"reload_removed", "removed_secret", "rw"};
    const User added{"reload_added", "added_secret", "rw"};
    const User denied{"reload_denied", "denied_secret", "rw"};

    const std::array<std::string, 3> names     = {"ECF_PASSWD", "ECF_CUSTOM_PASSWD", "ECF_LISTS"};
    const std::array<std::string, 3> filenames = {"active.passwd", "active.custom_passwd", "active.lists"};
    const std::array<std::string, 3> initial   = {
        PasswordsFile{host, port, regular}.data(),
        index == 2 ? PasswordsFile{host, port, controller, probe, removed, added, denied}.data()
                     : PasswordsFile{host, port, controller, probe, removed, denied}.data(),
        index == 2 ? WhitelistFile{regular, controller, probe, removed}.data()
                     : WhitelistFile{regular, controller, probe, removed, added}.data()};
    const std::array<std::string, 3> updated = {
        PasswordsFile{host, port, rotated_regular}.data(),
        PasswordsFile{host, port, controller, rotated_probe, added, denied}.data(),
        WhitelistFile{regular, controller, read_only_probe, added}.data()};

    std::string config = "ECF_HOME = " + home.string() + "\nECF_LOG = server.log\nECF_CHECKMODE = CHECK_NEVER\n";
    std::list<WithTestFile> files;
    std::list<WithTestEnvironmentVariable> overrides;
    for (size_t i = 0; i < names.size(); ++i) {
        files.emplace_back(NamedTestFile{(home / filenames[i]).string()}, initial[i]);
        if (from_environment) {
            overrides.emplace_back(names[i], filenames[i]);
        }
        else {
            config += names[i] + " = " + filenames[i] + "\n";
        }
    }
    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

    Process process(fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server",
                    {"--port=" + std::to_string(port.value())},
                    startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };

    boost::asio::io_context io;
    boost::system::error_code error;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    do {
        boost::asio::ip::tcp::socket socket(io);
        socket.connect({boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(port.value())}, error);
        if (!error) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    } while (std::chrono::steady_clock::now() < deadline);
    BOOST_REQUIRE_MESSAGE(!error, "Reload fixture server did not start: " << error.message());

    ClientInvoker client;
    client.set_throw_on_error(false);
    client.set_host_port(host.value(), std::to_string(port.value()));
    client.set_connection_attempts(1);
    client.set_connect_timeout(std::chrono::seconds(5));

    check_reload_access(client, regular, false, true);
    check_reload_access(client, probe, true, true);
    check_reload_access(client, probe, true, true, true);
    check_reload_access(client, removed, true, true);
    check_reload_access(client, added, true, false, false, index == 2 ? "Authorisation" : "Authentication");
    check_reload_access(client, denied, true, false, false, "Authorisation");

    const auto selected   = home / filenames[index];
    const bool successful = action == "config" || action == "update";
    BOOST_SCOPE_EXIT_ALL(&) {
        if (action == "unreadable") {
            std::error_code permission_error;
            fs::permissions(
                selected, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, permission_error);
            BOOST_CHECK_MESSAGE(!permission_error,
                                "Unable to restore fixture permissions: " << permission_error.message());
        }
    };

    if (action == "config") {
        std::string replacement_config = "ECF_HOME = " + home.string() + "\n";
        for (size_t i = 0; i < names.size(); ++i) {
            const auto alternate = "alternate." + filenames[i];
            files.emplace_back(NamedTestFile{(home / alternate).string()}, initial[i]);
            replacement_config += names[i] + " = " + alternate + "\n";
        }
        rewrite_reload_fixture(configuration.path(), replacement_config);
        rewrite_reload_fixture(selected, updated[index]);
    }
    else if (action == "update") {
        rewrite_reload_fixture(selected, updated[index]);
    }
    else if (action == "removed") {
        BOOST_REQUIRE(fs::remove(selected));
    }
    else if (action == "directory") {
        BOOST_REQUIRE(fs::remove(selected));
        BOOST_REQUIRE(fs::create_directory(selected));
    }
    else if (action == "unreadable") {
        fs::permissions(selected, fs::perms::none, fs::perm_options::replace);
        if (::access(selected.c_str(), R_OK) == 0) {
            BOOST_TEST_MESSAGE(
                "Skipping unreadable reload fixture: read restrictions are not enforced for this account");
            return;
        }
    }
    else if (action == "malformed_first") {
        rewrite_reload_fixture(selected, "not_a_version\n");
    }
    else {
        const auto partial = index == 2 ? WhitelistFile{controller, denied}.data() + "first_user second_user\n"
                                        : updated[index] + "incomplete password entry\n";
        rewrite_reload_fixture(selected, partial);
    }

    const auto reload = [&] {
        client.set_user_name(controller.username);
        client.set_password(PasswordEncryption::encrypt(controller.password, controller.username));
        return index == 0   ? client.reloadpasswdfile()
               : index == 1 ? client.reloadcustompasswdfile()
                            : client.reloadwsfile();
    };
    const int status = reload();
    BOOST_CHECK_MESSAGE((status == 0) == successful, client.errorMsg());
    if (!successful) {
        BOOST_CHECK_MESSAGE(client.errorMsg().find(filenames[index]) != std::string::npos, client.errorMsg());
        if (action == "removed") {
            BOOST_CHECK_MESSAGE(client.errorMsg().find("does not exist") != std::string::npos, client.errorMsg());
        }
        else if (action == "directory") {
            BOOST_CHECK_MESSAGE(client.errorMsg().find("not a regular file") != std::string::npos, client.errorMsg());
        }
        else if (action == "unreadable") {
            BOOST_CHECK_MESSAGE(client.errorMsg().find("Could not open") != std::string::npos, client.errorMsg());
        }
    }

    check_reload_access(
        client, regular, false, !(successful && index == 0), false, successful && index == 0 ? "Authentication" : "");
    check_reload_access(client, rotated_regular, false, successful && index == 0, false, "Authentication");
    check_reload_access(
        client, probe, true, !(successful && index == 1), false, successful && index == 1 ? "Authentication" : "");
    check_reload_access(client, rotated_probe, true, successful && index == 1, false, "Authentication");
    if (index == 2) {
        check_reload_access(client, probe, true, !successful, true, "Authorisation");
    }
    check_reload_access(
        client, removed, true, !(successful && index != 0), false, index == 2 ? "Authorisation" : "Authentication");
    check_reload_access(
        client, added, true, successful && index != 0, false, index == 2 ? "Authorisation" : "Authentication");
    check_reload_access(client, denied, true, false, false, "Authorisation");

    check_reload_access(client, controller, true, true);
    BOOST_REQUIRE(client.defs());
    for (size_t i = 0; i < names.size(); ++i) {
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(names[i]), filenames[i]);
    }

    if (!successful) {
        if (action == "directory") {
            BOOST_REQUIRE(fs::remove(selected));
        }
        else if (action == "unreadable") {
            fs::permissions(selected, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
        }
        rewrite_reload_fixture(selected, updated[index]);
        BOOST_REQUIRE_MESSAGE(reload() == 0, client.errorMsg());
        if (index == 0) {
            check_reload_access(client, regular, false, false, false, "Authentication");
            check_reload_access(client, rotated_regular, false, true);
        }
        else if (index == 1) {
            check_reload_access(client, probe, true, false, false, "Authentication");
            check_reload_access(client, rotated_probe, true, true);
            check_reload_access(client, removed, true, false, false, "Authentication");
            check_reload_access(client, added, true, true);
        }
        else {
            check_reload_access(client, probe, true, true);
            check_reload_access(client, probe, true, false, true, "Authorisation");
            check_reload_access(client, removed, true, false, false, "Authorisation");
            check_reload_access(client, added, true, true);
        }
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerReload)

///
/// @brief Verifies that changed configuration paths do not redirect any security reload command.
///
BOOST_AUTO_TEST_CASE(test_server_reload_keeps_selected_paths) {
    ECF_NAME_THIS_TEST();

    for (size_t index = 0; index < 3; ++index) {
        for (const bool from_environment : {false, true}) {
            BOOST_TEST_CONTEXT("Security index " << index << ", environment source: " << from_environment) {
                check_security_reload(index, "config", from_environment);
            }
        }
    }
}

///
/// @brief Verifies that original-file updates apply changed passwords, membership and permissions.
///
BOOST_AUTO_TEST_CASE(test_server_reload_applies_original_file_updates) {
    ECF_NAME_THIS_TEST();

    for (size_t index = 0; index < 3; ++index) {
        for (const bool from_environment : {false, true}) {
            BOOST_TEST_CONTEXT("Security index " << index << ", environment source: " << from_environment) {
                check_security_reload(index, "update", from_environment);
            }
        }
    }
}

///
/// @brief Verifies failed reloads of removed files preserve active credentials and permissions.
///
BOOST_AUTO_TEST_CASE(test_server_reload_removed_file_preserves_state) {
    ECF_NAME_THIS_TEST();

    for (size_t index = 0; index < 3; ++index) {
        for (const bool from_environment : {false, true}) {
            BOOST_TEST_CONTEXT("Security index " << index << ", environment source: " << from_environment) {
                check_security_reload(index, "removed", from_environment);
            }
        }
    }
}

///
/// @brief Verifies failed reloads preserve state after malformed, unreadable and directory-valued replacements.
///
BOOST_AUTO_TEST_CASE(test_server_reload_malformed_file_preserves_state) {
    ECF_NAME_THIS_TEST();

    for (size_t index = 0; index < 3; ++index) {
        for (const bool from_environment : {false, true}) {
            for (const std::string action : {"malformed_first", "malformed_partial", "directory", "unreadable"}) {
                BOOST_TEST_CONTEXT("Security index " << index << ", environment source: " << from_environment
                                                     << ", content: " << action) {
                    check_security_reload(index, action, from_environment);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
