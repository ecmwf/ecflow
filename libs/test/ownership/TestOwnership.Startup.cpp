// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>

#include "PrivilegedEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf::test::ownership;

BOOST_AUTO_TEST_SUITE(S_Ownership)

BOOST_AUTO_TEST_SUITE(T_Startup)

namespace {

///
/// Launches a server expected to refuse to start, and checks the reason it gives.
///
void check_refused(const Launch& launch, const std::string& expected_reason) {
    LaunchedServer server(launch);
    const auto status = server.wait_for_exit();
    const auto output = server.stop();
    BOOST_REQUIRE_MESSAGE(status.has_value(), "The server did not refuse to start; output:\n" << output);
    BOOST_CHECK_NE(*status, 0);
    BOOST_CHECK_MESSAGE(output.find("Ownership: the server cannot start: " + expected_reason) != std::string::npos,
                        "Expected '" << expected_reason << "'; output:\n"
                                     << output);
}

///
/// A copy of the server executable, belonging to the server account, removed on destruction.
///
class ExecutableCopy {
public:
    explicit ExecutableCopy(mode_t mode)
        : directory_{server_account} {
        // The directory must be traversable by the accounts being granted access
        ::chmod(directory_.path().c_str(), 0755);
        path_ = directory_.path() / "ecflow_server";
        fs::copy_file(fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server", path_);
        const auto [u, g] = ids_of(server_account);
        BOOST_REQUIRE(::chown(path_.c_str(), u, g) == 0);
        BOOST_REQUIRE(::chmod(path_.c_str(), mode) == 0);
    }

    /// Grants CAP_SETUID and CAP_SETGID to the executable, as permitted file capabilities
    void grant_file_capabilities() const {
        const auto [status, output] = run(required_executable("setcap"), {"cap_setuid,cap_setgid=p", path_.string()});
        BOOST_REQUIRE_MESSAGE(status == 0, "setcap failed: " << output);
    }

    /// Adds an entry for another account to the ACL, with a mask that keeps the group execute bit clear, so that only
    /// the presence of the ACL makes the executable unsafe
    void add_acl_entry_for(const std::string& account) const {
        const auto [status, output] =
            run(required_executable("setfacl"), {"-m", "u:" + account + ":rx,m::r", path_.string()});
        BOOST_REQUIRE_MESSAGE(status == 0, "setfacl failed: " << output);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    AccountDirectory directory_;
    fs::path path_;
};

} // namespace

BOOST_AUTO_TEST_CASE(test_starts_with_ambient_capabilities) {
    ECF_NAME_THIS_TEST();

    LaunchedServer server(Launch{});
    server.require_ready();

    ClientInvoker client;
    server.configure(client, alice);
    BOOST_REQUIRE_MESSAGE(client.stats() == 0, client.errorMsg());
    BOOST_CHECK_MESSAGE(client.get_string().find("enabled, server Owner " + server_owner +
                                                 ", capabilities CAP_SETUID and CAP_SETGID") != std::string::npos,
                        client.get_string());

    BOOST_REQUIRE_MESSAGE(client.ownership("read", "/") == 0, client.errorMsg());
    BOOST_CHECK_EQUAL(client.get_string(), "effective=" + server_owner + "\norigin=server\n");
}

BOOST_AUTO_TEST_CASE(test_starts_as_root_with_a_warning) {
    ECF_NAME_THIS_TEST();

    Launch launch;
    launch.account = "root";
    LaunchedServer server(launch);
    server.require_ready();

    ClientInvoker client;
    server.configure(client, alice);
    BOOST_REQUIRE_MESSAGE(client.stats() == 0, client.errorMsg());
    BOOST_CHECK_MESSAGE(client.get_string().find("enabled, server Owner " + server_owner + ", running as root") !=
                            std::string::npos,
                        client.get_string());

    const auto log = server.log();
    BOOST_CHECK_MESSAGE(log.find("WAR:") != std::string::npos &&
                            log.find("Ownership: the server runs as root, which is not recommended") !=
                                std::string::npos,
                        log);
}

BOOST_AUTO_TEST_CASE(test_refused_without_capabilities) {
    ECF_NAME_THIS_TEST();

    Launch launch;
    launch.ambient_capabilities = false;
    check_refused(launch, "changing identity requires the capabilities CAP_SETUID and CAP_SETGID, or running as root");
}

BOOST_AUTO_TEST_CASE(test_refused_with_the_server_account_as_server_owner) {
    ECF_NAME_THIS_TEST();

    Launch launch;
    launch.owner = server_account;
    check_refused(launch, "Invalid server Owner: The owner '" + server_account + "' is the server account");
}

BOOST_AUTO_TEST_CASE(test_starts_with_private_file_capabilities) {
    ECF_NAME_THIS_TEST();

    // The capabilities are only granted through the file, which only the server account can execute
    ExecutableCopy executable(0700);
    executable.grant_file_capabilities();

    Launch launch;
    launch.ambient_capabilities = false;
    launch.executable           = executable.path();
    LaunchedServer server(launch);
    server.require_ready();
}

BOOST_AUTO_TEST_CASE(test_refused_with_file_capabilities_executable_by_others) {
    ECF_NAME_THIS_TEST();

    ExecutableCopy executable(0755);
    executable.grant_file_capabilities();

    Launch launch;
    launch.ambient_capabilities = false;
    launch.executable           = executable.path();
    check_refused(launch,
                  "the server executable is unsafe: it carries file capabilities and is executable by other accounts");
}

BOOST_AUTO_TEST_CASE(test_refused_with_file_capabilities_and_an_acl) {
    ECF_NAME_THIS_TEST();

    ExecutableCopy executable(0700);
    executable.grant_file_capabilities();
    executable.add_acl_entry_for(alice);
    BOOST_REQUIRE((fs::status(executable.path()).permissions() & fs::perms::group_exec) == fs::perms::none);

    Launch launch;
    launch.ambient_capabilities = false;
    launch.executable           = executable.path();
    check_refused(launch,
                  "the server executable is unsafe: it carries file capabilities and has an access control list");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
