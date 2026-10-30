// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <chrono>
#include <csignal>
#include <future>
#include <list>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

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
/// @brief Exercises security-file selection, validation and access through an isolated server executable.
///
/// @param[in] kinds Regular-password, custom-password and whitelist fixture kinds
/// @param[in] from_environment Whether environment-selected files compete with valid config-selected files
/// @param[in] rejected_index Index of the file expected to prevent startup, or -1 for successful startup
/// @param[in] cause Required failure diagnostic fragment when startup must fail
///
void check_server_security(const std::array<std::string, 3>& kinds,
                           bool from_environment    = false,
                           int rejected_index       = -1,
                           const std::string& cause = {}) {
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

    const User regular{ecf::get_login_name(), "regular_secret", "rw"};
    const User writer{"security_writer", "writer_secret", "rw"};
    const User reader{"security_reader", "reader_secret", "r"};
    const User excluded{"security_excluded", "excluded_secret", "rw"};
    const User config_user{"config_only_user", "config_secret", "rw"};
    const std::array<std::string, 3> names = {"ECF_PASSWD", "ECF_CUSTOM_PASSWD", "ECF_LISTS"};
    std::array<fs::path, 3> selected_paths;
    std::list<WithTestFile> files;
    std::list<WithTestEnvironmentVariable> overrides;

    BOOST_SCOPE_EXIT_ALL(&) {
        for (size_t i = 0; i < kinds.size(); ++i) {
            if (kinds[i] == "unreadable" && fs::exists(selected_paths[i])) {
                std::error_code error;
                fs::permissions(selected_paths[i],
                                fs::perms::owner_read | fs::perms::owner_write,
                                fs::perm_options::replace,
                                error);
                BOOST_CHECK_MESSAGE(!error, "Unable to restore security-file permissions: " << error.message());
            }
        }
    };

    std::string config = "ECF_HOME = " + home.string() + "\nECF_LOG = server.log\nECF_CHECKMODE = CHECK_NEVER\n";
    for (size_t i = 0; i < names.size(); ++i) {
        const auto selected = home / ("selected." + names[i]);
        selected_paths[i]   = selected;
        const auto content  = i == 0   ? PasswordsFile{host, port, regular}.data()
                              : i == 1 ? PasswordsFile{host, port, writer, reader, excluded}.data()
                                       : WhitelistFile{regular, writer, reader}.data();
        const auto& kind    = kinds[i];
        if (kind == "directory") {
            fs::create_directory(selected);
        }
        else if (kind != "missing") {
            std::string data = content;
            if (kind == "malformed") {
                data = "not_a_version\n";
            }
            else if (kind == "empty") {
                data.clear();
            }
            else if (kind == "version") {
                data = "4.5.0\n";
            }
            else if (kind == "wrong_host") {
                data = "4.5.0\nother_user other.invalid.host " + std::to_string(port.value()) + " secret\n";
            }
            else if (kind == "wrong_port") {
                data = "4.5.0\nother_user " + host.value() + " " +
                       std::to_string(port.value() == 49150 ? 49149 : port.value() + 1) + " secret\n";
            }

            files.emplace_back(NamedTestFile{selected.string()}, data);
            if (kind == "unreadable") {
                fs::permissions(selected, fs::perms::none, fs::perm_options::replace);
                if (::access(selected.c_str(), R_OK) == 0) {
                    ecf::test::scaffold::skip_unenforced_permissions("the unreadable " + names[i] + " case");
                    return;
                }
            }
        }

        if (from_environment) {
            const auto decoy = home / ("config." + names[i]);
            const auto decoy_content =
                i == 0   ? PasswordsFile{host, port, User{regular.username, "config_regular_secret", "rw"}}.data()
                : i == 1 ? PasswordsFile{host, port, config_user}.data()
                         : WhitelistFile{config_user}.data();
            files.emplace_back(NamedTestFile{decoy.string()}, decoy_content);
            config += names[i] + " = " + decoy.string() + "\n";
            overrides.emplace_back(names[i], selected.string());
        }
        else {
            config += names[i] + " = " + selected.string() + "\n";
        }
    }

    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

    Process process(fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server",
                    {"--port=" + std::to_string(port.value())},
                    startup.path());
    bool reaped = false;
    BOOST_SCOPE_EXIT_ALL(&) {
        if (!reaped) {
            process.terminate();
            process.wait();
        }
    };

    if (rejected_index >= 0) {
        auto completion = std::async(std::launch::async, [&] { return process.wait(); });
        const bool exited =
            completion.wait_for(ecf::test::scaffold::server_startup_timeout) == std::future_status::ready;
        if (!exited) {
            ::kill(process.pid(), SIGKILL);
        }
        const auto status = completion.get();
        reaped            = true;
        const auto output = process.read_stdout() + process.read_stderr();

        BOOST_REQUIRE_MESSAGE(exited, "Security failure did not stop startup: " << output);
        BOOST_CHECK_EQUAL(status, 1);
        BOOST_CHECK_MESSAGE(output.find(selected_paths[rejected_index].string()) != std::string::npos, output);
        BOOST_CHECK_MESSAGE(output.find(cause) != std::string::npos, output);

        boost::asio::io_context io;
        boost::asio::ip::tcp::socket socket(io);
        boost::system::error_code error;
        socket.connect({boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(port.value())}, error);
        BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                            "Security failure left a listener: " << error.message());
        return;
    }

    boost::asio::io_context io;
    boost::system::error_code error;
    const auto deadline = std::chrono::steady_clock::now() + ecf::test::scaffold::server_startup_timeout;
    do {
        boost::asio::ip::tcp::socket socket(io);
        socket.connect({boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(port.value())}, error);
        if (!error) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    } while (std::chrono::steady_clock::now() < deadline);
    BOOST_REQUIRE_MESSAGE(!error, "Valid security configuration did not start: " << error.message());

    ClientInvoker client;
    client.set_throw_on_error(false);
    client.set_host_port(host.value(), std::to_string(port.value()));
    client.set_connection_attempts(1);
    client.set_connect_timeout(std::chrono::seconds(5));

    const bool regular_present = kinds[0] != "missing";
    client.set_user_name("");
    client.set_password(regular_present ? PasswordEncryption::encrypt(regular.password, regular.username) : "");
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());
    for (size_t i = 0; i < names.size(); ++i) {
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(names[i]), selected_paths[i].string());
    }

    client.set_password(PasswordEncryption::encrypt("incorrect_regular_secret", regular.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());

    const bool custom_present = kinds[1] != "missing";
    const bool restricted     = kinds[2] == "valid";
    client.set_user_name(writer.username);
    client.set_password(PasswordEncryption::encrypt(writer.password, writer.username));
    if (!custom_present) {
        BOOST_CHECK_NE(client.getDefs(), 0);
        BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());
    }
    else {
        BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
        BOOST_CHECK_MESSAGE(client.checkPtDefs(ecf::CheckPt::NEVER) == 0, client.errorMsg());

        client.set_password(PasswordEncryption::encrypt("incorrect_custom_secret", writer.username));
        BOOST_CHECK_NE(client.getDefs(), 0);
        BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());

        client.set_user_name(reader.username);
        client.set_password(PasswordEncryption::encrypt(reader.password, reader.username));
        BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
        BOOST_CHECK_EQUAL(client.checkPtDefs(ecf::CheckPt::NEVER) == 0, !restricted);
        if (restricted) {
            BOOST_CHECK_MESSAGE(client.errorMsg().find("Authorisation") != std::string::npos, client.errorMsg());
        }

        client.set_user_name(excluded.username);
        client.set_password(PasswordEncryption::encrypt(excluded.password, excluded.username));
        BOOST_CHECK_EQUAL(client.getDefs() == 0, !restricted);
        if (restricted) {
            BOOST_CHECK_MESSAGE(client.errorMsg().find("Authorisation") != std::string::npos, client.errorMsg());
        }
    }

    if (from_environment) {
        client.set_user_name(config_user.username);
        client.set_password(PasswordEncryption::encrypt(config_user.password, config_user.username));
        BOOST_CHECK_NE(client.getDefs(), 0);
        BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());

        client.set_user_name("");
        client.set_password(PasswordEncryption::encrypt("config_regular_secret", regular.username));
        BOOST_CHECK_NE(client.getDefs(), 0);
        BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerSecurity)

///
/// @brief Verifies accepted and refused read/write requests using config-selected security files.
///
BOOST_AUTO_TEST_CASE(test_server_security_config_selection) {
    ECF_NAME_THIS_TEST();

    check_server_security({"valid", "valid", "valid"});
}

///
/// @brief Verifies that environment-selected credentials and permissions replace conflicting valid config files.
///
BOOST_AUTO_TEST_CASE(test_server_security_environment_selection) {
    ECF_NAME_THIS_TEST();

    check_server_security({"valid", "valid", "valid"}, true);
}

///
/// @brief Verifies all eight presence combinations and independent regular/custom authentication without a whitelist.
///
BOOST_AUTO_TEST_CASE(test_server_security_missing_combinations) {
    ECF_NAME_THIS_TEST();

    for (unsigned mask = 0; mask < 8; ++mask) {
        BOOST_TEST_CONTEXT("Security-file presence mask: " << mask) {
            check_server_security(
                {mask & 1 ? "valid" : "missing", mask & 2 ? "valid" : "missing", mask & 4 ? "valid" : "missing"});
        }
    }
}

///
/// @brief Verifies rejection of malformed, empty and version-only password files while the other file remains valid.
///
BOOST_AUTO_TEST_CASE(test_server_security_invalid_password_content) {
    ECF_NAME_THIS_TEST();

    for (int index = 0; index < 2; ++index) {
        for (const std::string kind : {"malformed", "empty", "version"}) {
            BOOST_TEST_CONTEXT("Password index " << index << ", content: " << kind) {
                std::array<std::string, 3> kinds = {"valid", "valid", "valid"};
                kinds[index]                     = kind;
                check_server_security(
                    kinds, false, index, kind == "malformed" ? "could not be parsed" : "does not contain any users");
            }
        }
    }
}

///
/// @brief Verifies malformed-whitelist rejection and unrestricted empty or version-only whitelist controls.
///
BOOST_AUTO_TEST_CASE(test_server_security_whitelist_content) {
    ECF_NAME_THIS_TEST();

    check_server_security({"valid", "valid", "malformed"}, false, 2, "ECF_LISTS");
    check_server_security({"valid", "valid", "empty"});
    check_server_security({"valid", "valid", "version"});
}

///
/// @brief Verifies rejection of each password-file kind containing only another host or port.
///
BOOST_AUTO_TEST_CASE(test_server_security_password_host_port_mismatch) {
    ECF_NAME_THIS_TEST();

    for (int index = 0; index < 2; ++index) {
        for (const std::string kind : {"wrong_host", "wrong_port"}) {
            BOOST_TEST_CONTEXT("Password index " << index << ", mismatch: " << kind) {
                std::array<std::string, 3> kinds = {"valid", "valid", "valid"};
                kinds[index]                     = kind;
                check_server_security(kinds, false, index, "does not contain any users");
            }
        }
    }
}

///
/// @brief Verifies controlled rejection of directory-valued paths for all three security files.
///
BOOST_AUTO_TEST_CASE(test_server_security_directory_paths) {
    ECF_NAME_THIS_TEST();

    for (int index = 0; index < 3; ++index) {
        BOOST_TEST_CONTEXT("Directory-valued security index: " << index) {
            std::array<std::string, 3> kinds = {"valid", "valid", "valid"};
            kinds[index]                     = "directory";
            check_server_security(kinds, false, index, "not a regular file");
        }
    }
}

///
/// @brief Verifies controlled rejection of unreadable security files with explicit permission-enforcement skips.
///
BOOST_AUTO_TEST_CASE(test_server_security_unreadable_paths) {
    ECF_NAME_THIS_TEST();

    for (int index = 0; index < 3; ++index) {
        BOOST_TEST_CONTEXT("Unreadable security index: " << index) {
            std::array<std::string, 3> kinds = {"valid", "valid", "valid"};
            kinds[index]                     = "unreadable";
            check_server_security(kinds, false, index, "Could not open");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
