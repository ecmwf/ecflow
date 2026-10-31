// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <list>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/File.hpp"
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

    ecf::test::foolproof::ClearedServerEnvironment cleared;

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

    const auto decoy_log = startup.path() / fs::relative(destinations[0], home);
    if (form != "absolute") {
        fs::create_directories(decoy_log.parent_path());
        files.emplace_back(NamedTestFile{decoy_log.string()}, "unchanged startup-side log\n");
    }

    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

    auto server = MakeServer{}.with(host).with(port).with(startup).launch();
    BOOST_REQUIRE_MESSAGE(server.ok(), (server.ok() ? "" : server.reason()));

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());
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

    for (size_t i = 1; i < 3; ++i) {
        if (form != "absolute") {
            BOOST_CHECK(!fs::exists(startup.path() / fs::relative(destinations[i], home)));
        }
    }

    if (form != "absolute") {
        std::vector<std::string> lines;
        BOOST_REQUIRE(ecf::File::splitFileIntoLines(decoy_log.string(), lines));
        BOOST_REQUIRE_EQUAL(lines.size(), 1);
        BOOST_CHECK_EQUAL(lines.front(), "unchanged startup-side log");
    }

    client.set_password(PasswordEncryption::encrypt("incorrect_secret", allowed.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());

    client.set_user_name(excluded.username);
    client.set_password(PasswordEncryption::encrypt(excluded.password, excluded.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authorisation") != std::string::npos, client.errorMsg());

    client.set_user_name("");
    client.set_password(PasswordEncryption::encrypt(regular.password, regular.username));
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());

    client.set_password(PasswordEncryption::encrypt("incorrect_regular_secret", regular.username));
    BOOST_CHECK_NE(client.getDefs(), 0);
    BOOST_CHECK_MESSAGE(client.errorMsg().find("Authentication") != std::string::npos, client.errorMsg());
}

///
/// @brief Checks clean executable rejection of an unusable server home from either supported source.
///
/// @param[in] failure Missing, file, empty, read, write or search failure to provision
/// @param[in] from_environment Whether ECF_HOME is selected through an environment override
///
void check_server_home_failure(const std::string& failure, bool from_environment) {
    using namespace ecf::test::scaffold;

    auto root       = MakeDirectory{}.create();
    const auto home = root.path() / "unusable home";
    const auto log  = root.path() / "server.log";

    BOOST_SCOPE_EXIT_ALL(&) {
        if (fs::is_directory(home)) {
            std::error_code error;
            fs::permissions(home, fs::perms::owner_all, fs::perm_options::replace, error);
            BOOST_CHECK_MESSAGE(!error, "Unable to restore home permissions: " << error.message());
        }
    };

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    std::list<WithTestFile> files;
    if (failure == "file") {
        files.emplace_back(NamedTestFile{home.string()}, "not a directory\n");
    }
    else if (failure == "read" || failure == "write" || failure == "search") {
        fs::create_directories(home);
        const auto removed = failure == "read"    ? fs::perms::owner_read
                             : failure == "write" ? fs::perms::owner_write
                                                  : fs::perms::owner_exec;
        fs::permissions(home, fs::perms::owner_all & ~removed, fs::perm_options::replace);
        const int mode = failure == "read" ? R_OK : failure == "write" ? W_OK : X_OK;
        if (::access(home.c_str(), mode) == 0) {
            ecf::test::scaffold::skip_unenforced_permissions("the " + failure + " permission case");
            return;
        }
    }

    const auto selected = failure == "empty" ? std::string{} : home.string();
    std::string config  = "ECF_LOG = " + log.string() + "\n";
    std::list<WithTestEnvironmentVariable> overrides;
    if (from_environment) {
        overrides.emplace_back("ECF_HOME", selected);
    }
    else {
        config += "ECF_HOME = " + selected + "\n";
    }

    WithTestFile configuration(NamedTestFile{(root.path() / "server_environment.cfg").string()}, config);

    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, root.path());
    const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
    BOOST_REQUIRE_MESSAGE(exited, "Invalid home did not cause bounded startup rejection: " << output);
    BOOST_CHECK_EQUAL(status, 1);
    BOOST_CHECK_MESSAGE(output.find("ECF_HOME") != std::string::npos, output);
    if (!selected.empty()) {
        BOOST_CHECK_MESSAGE(output.find(selected) != std::string::npos, output);
    }
    // Other failures are identified by the setting and the path, not by the wording of the cause
    if (failure == "empty") {
        BOOST_CHECK_MESSAGE(output.find("must not be empty") != std::string::npos, output);
    }
    BOOST_CHECK(!fs::exists(log));

    const auto error = ecf::test::foolproof::probe_no_listener(port.value());
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Expected connection refusal after failed startup, found: " << error.message());
}

///
/// @brief Checks a home selected relative to the startup directory in an isolated server process.
///
/// @param[in] source Default, config or environment selection of the home
/// @param[in] home Relative home to select; ignored for the default source, which selects the startup directory
///
void check_server_relative_home(const std::string& source, const std::string& home = ".") {
    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();
    // The exported home is absolute and in normal form, without dot segments or a trailing separator
    auto expected_home = (startup.path() / (source == "default" ? std::string(".") : home)).lexically_normal();
    if (expected_home.filename().empty()) {
        expected_home = expected_home.parent_path();
    }
    fs::create_directories(expected_home);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()},
                               source == "config" ? "ECF_HOME = " + home + "\n" : std::string());

    std::list<WithTestEnvironmentVariable> overrides;
    if (source == "environment") {
        overrides.emplace_back("ECF_HOME", home);
    }

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto server = MakeServer{}.with(host).with(port).with(startup).launch();
    BOOST_REQUIRE_MESSAGE(server.ok(), (server.ok() ? "" : server.reason()));

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());

    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_HOME"), expected_home.string());

    const auto prefix = host.value() + "." + std::to_string(port.value()) + ".";
    for (const auto& [name, suffix] : std::vector<std::pair<std::string, std::string>>{
             {"ECF_LOG", "ecf.log"}, {"ECF_CHECK", "ecf.check"}, {"ECF_CHECKOLD", "ecf.check.b"}}) {
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(name),
                          (expected_home / (prefix + suffix)).string());
    }

    BOOST_REQUIRE_MESSAGE(client.checkPtDefs() == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.checkPtDefs() == 0, client.errorMsg());

    for (const auto& suffix : {"ecf.log", "ecf.check", "ecf.check.b"}) {
        BOOST_CHECK(fs::is_regular_file(expected_home / (prefix + suffix)));
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerPaths)

///
/// @brief Verifies actual file access and exported definition paths for config-selected filenames.
///
BOOST_AUTO_TEST_CASE(test_server_paths_filenames_from_config) {
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
BOOST_AUTO_TEST_CASE(test_server_paths_filenames_from_environment) {
    ECF_NAME_THIS_TEST();

    for (const auto& form : {"relative", "absolute", "spaces"}) {
        BOOST_TEST_CONTEXT("Path form: " << form) {
            check_server_runtime_paths(true, form);
        }
    }
}

///
/// @brief Verifies absent and explicit dot homes without introducing an extra directory level.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_default_and_dot) {
    ECF_NAME_THIS_TEST();

    for (const auto& source : {"default", "config", "environment"}) {
        BOOST_TEST_CONTEXT("Home source: " << source) {
            check_server_relative_home(source);
        }
    }
}

///
/// @brief Verifies that a relative home with dot segments and a trailing separator is used and exported in normal
/// form, relative to the startup directory.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_relative) {
    ECF_NAME_THIS_TEST();

    for (const auto& source : {"config", "environment"}) {
        BOOST_TEST_CONTEXT("Home source: " << source) {
            check_server_relative_home(source, "./server home/");
        }
    }
}

///
/// @brief Verifies clean rejection of a nonexistent home from configuration and environment inputs.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_missing) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("missing", from_environment);
        }
    }
}

///
/// @brief Verifies clean rejection of a regular file selected as server home.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_regular_file) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("file", from_environment);
        }
    }
}

///
/// @brief Verifies that an explicitly empty home fails without an assertion or signal.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_empty) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("empty", from_environment);
        }
    }
}

///
/// @brief Verifies startup rejection when server home lacks read permission.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_without_read_permission) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("read", from_environment);
        }
    }
}

///
/// @brief Verifies startup rejection when server home lacks write permission.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_without_write_permission) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("write", from_environment);
        }
    }
}

///
/// @brief Verifies startup rejection when server home lacks search permission.
///
BOOST_AUTO_TEST_CASE(test_server_paths_home_without_search_permission) {
    ECF_NAME_THIS_TEST();

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << from_environment) {
            check_server_home_failure("search", from_environment);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
