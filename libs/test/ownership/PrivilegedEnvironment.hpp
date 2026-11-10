// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <grp.h>
#include <iterator>
#include <list>
#include <optional>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

extern char** environ; // NOLINT(readability-redundant-declaration)
#include <vector>

#include <boost/test/unit_test.hpp>
#include <sys/stat.h>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

///
/// Helpers of the privileged ownership tests (s_ownership), which run in the privileged test image only (see
/// docker/README.md): the tests run as root, and launch servers under the accounts prepared by the image.
///
namespace ecf::test::ownership {

/// The environment variable set by the privileged test image
inline constexpr const char* image_marker = "ECF_TEST_PRIVILEGED_IMAGE";

/// The file created by the privileged test image, which marks it even when the environment variable is lost
inline constexpr const char* image_marker_file = "/etc/ecflow-privileged-test-image";

/// The exit status of a test reporting itself skipped (registered as SKIP_RETURN_CODE)
inline constexpr int skipped_exit_status = 77;

/// The account running the servers, which holds CAP_SETUID and CAP_SETGID
inline const std::string server_account = "ecflow";

/// The non-privileged account used as server Owner
inline const std::string server_owner = "ecfdefault";

/// The accounts of the owners
inline const std::string alice   = "alice";
inline const std::string bob     = "bob";
inline const std::string charlie = "charlie";

/// The group of all the owners
inline const std::string owners_group = "ecflow-owners";

/// The root of the shared tree prepared by the image (server account, group of the owners, mode 3770)
inline const std::string shared_root = "/srv/ecflow";

/// The password of every ecFlow user of the tests
inline const std::string password = "ownership_secret";

///
/// @brief Reports whether the tests run in the privileged test image.
///
/// @return True when the marker variable is set, or the marker file exists: in the image, the tests are never skipped.
///
inline bool in_privileged_image() {
    return std::getenv(image_marker) != nullptr || ::access(image_marker_file, F_OK) == 0;
}

///
/// @brief Removes from the environment of the process every ecFlow setting (ECF_*), except those of the tests
///        (ECF_TEST_*).
///
inline void clear_inherited_ecflow_settings() {
    std::vector<std::string> names;
    for (char** entry = environ; entry && *entry; ++entry) {
        const std::string variable = *entry;
        const std::string name     = variable.substr(0, variable.find('='));
        if (name.rfind("ECF_", 0) == 0 && name.rfind("ECF_TEST_", 0) != 0) {
            names.push_back(name);
        }
    }
    for (const auto& name : names) {
        ::unsetenv(name.c_str());
    }
}

///
/// @brief Finds an executable in the PATH, or in the system administration directories.
///
/// @param[in] name The name of the executable
/// @return The path of the executable, or no value when it is not found.
///
inline std::optional<fs::path> find_executable(const std::string& name) {
    std::string directories = std::getenv("PATH") ? std::getenv("PATH") : "";
    directories += ":/usr/sbin:/sbin:/usr/bin:/bin";
    std::string::size_type begin = 0;
    while (begin <= directories.size()) {
        const auto end = std::min(directories.find(':', begin), directories.size());
        if (end > begin) {
            const auto candidate = fs::path(directories.substr(begin, end - begin)) / name;
            if (::access(candidate.c_str(), X_OK) == 0) {
                return candidate;
            }
        }
        begin = end + 1;
    }
    return std::nullopt;
}

///
/// @brief Finds an executable required by the tests.
///
/// @param[in] name The name of the executable
/// @return The path of the executable.
/// @throws std::runtime_error if it is not found (a missing prerequisite of the image).
///
inline fs::path required_executable(const std::string& name) {
    if (auto found = find_executable(name); found) {
        return *found;
    }
    throw std::runtime_error("The executable " + name + " is not available");
}

///
/// @brief Returns the user and group ids of an account.
///
/// @param[in] account The name of the account
/// @return The user id and the primary group id.
/// @throws std::runtime_error if the account does not exist.
///
inline std::pair<uid_t, gid_t> ids_of(const std::string& account) {
    const passwd* entry = ::getpwnam(account.c_str());
    if (entry == nullptr) {
        throw std::runtime_error("The account " + account + " does not exist");
    }
    return {entry->pw_uid, entry->pw_gid};
}

///
/// @brief Lists the prerequisites of the privileged tests missing from the current environment.
///
/// @return A description of each missing prerequisite; empty when all are present.
///
inline std::vector<std::string> missing_prerequisites() {
    std::vector<std::string> missing;
#ifndef ECF_OWNERSHIP_UNDER_TEST
    missing.emplace_back("ecFlow built with ENABLE_OWNERSHIP_UNDER_TEST=ON");
#endif
    if (::geteuid() != 0) {
        missing.emplace_back("the tests running as root");
    }
    for (const auto& account : {server_account, server_owner, alice, bob, charlie}) {
        if (::getpwnam(account.c_str()) == nullptr) {
            missing.emplace_back("the account " + account);
        }
    }
    const group* owners = ::getgrnam(owners_group.c_str());
    if (owners == nullptr) {
        missing.emplace_back("the group " + owners_group);
    }
    else {
        for (const auto& account : {server_owner, alice, bob, charlie}) {
            bool member = false;
            for (char** m = owners->gr_mem; m && *m; ++m) {
                member = member || account == *m;
            }
            if (!member) {
                missing.emplace_back("the account " + account + " in the group " + owners_group);
            }
        }
    }
    for (const auto* executable : {"setpriv", "setcap", "setfacl"}) {
        if (!find_executable(executable)) {
            missing.emplace_back("the executable " + std::string(executable));
        }
    }
    struct stat root = {};
    if (::lstat(shared_root.c_str(), &root) != 0 || !S_ISDIR(root.st_mode)) {
        missing.emplace_back("the shared tree " + shared_root + " (a directory)");
    }
    else if (::getpwnam(server_account.c_str()) && owners &&
             (root.st_uid != ::getpwnam(server_account.c_str())->pw_uid || root.st_gid != owners->gr_gid ||
              (root.st_mode & 07777) != 03770)) {
        missing.emplace_back("the shared tree " + shared_root + " owned by " + server_account + ":" + owners_group +
                             " with mode 3770");
    }
    return missing;
}

///
/// @brief Creates, for the lifetime of the object, a private directory belonging to an account.
///
class AccountDirectory {
public:
    ///
    /// @brief Creates the directory, with mode 0700.
    ///
    /// @param[in] account The account owning the directory
    /// @throws std::runtime_error if the directory cannot be created.
    ///
    explicit AccountDirectory(const std::string& account) {
        std::string pattern = "/var/tmp/ecf-ownership-XXXXXX";
        if (::mkdtemp(pattern.data()) == nullptr) {
            throw std::runtime_error("Unable to create a directory from " + pattern);
        }
        path_ = pattern;
        try {
            const auto [u, g] = ids_of(account);
            if (::chown(path_.c_str(), u, g) != 0 || ::chmod(path_.c_str(), 0700) != 0) {
                throw std::runtime_error("Unable to give " + path_.string() + " to " + account);
            }
        }
        catch (...) {
            // The destructor does not run for an object whose construction fails
            std::error_code ignored;
            fs::remove_all(path_, ignored);
            throw;
        }
    }

    AccountDirectory(const AccountDirectory&)            = delete;
    AccountDirectory& operator=(const AccountDirectory&) = delete;

    ~AccountDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

///
/// @brief Writes a file belonging to an account.
///
/// @param[in] path The path of the file
/// @param[in] content The content of the file
/// @param[in] account The account owning the file
/// @param[in] mode The permissions of the file
/// @throws std::runtime_error if the file cannot be written.
///
inline void write_file_of(const fs::path& path, const std::string& content, const std::string& account, mode_t mode) {
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << content;
        if (!output) {
            throw std::runtime_error("Unable to write " + path.string());
        }
    }
    const auto [u, g] = ids_of(account);
    if (::chown(path.c_str(), u, g) != 0 || ::chmod(path.c_str(), mode) != 0) {
        throw std::runtime_error("Unable to give " + path.string() + " to " + account);
    }
}

///
/// @brief How a server is launched.
///
struct Launch
{
    std::string account       = server_account;      ///< The account running the server; root runs it directly
    bool ambient_capabilities = true;                ///< Whether CAP_SETUID and CAP_SETGID are granted as ambient
    fs::path executable;                             ///< The server executable; the one of the build when empty
    std::optional<std::string> owner = server_owner; ///< The server Owner (--ownership_owner), if any
};

///
/// @brief A server launched under an account, with its home, its port and the passwords of the test users.
///
/// The server is killed on destruction (it ignores SIGTERM).
///
class LaunchedServer {
public:
    ///
    /// @brief Launches the server, without waiting for it to listen.
    ///
    /// @param[in] launch How the server is launched
    /// @throws std::runtime_error if a prerequisite of the launch is missing.
    ///
    explicit LaunchedServer(const Launch& launch)
        : home_{launch.account},
          host_{ecf::test::scaffold::MakeHost{}.create()},
          port_{reserve_port()} {
        // Every user of the tests (and the plugging server, forwarding them) authenticates with the same password
        std::string passwords = std::string(ECFLOW_VERSION) + "\n";
        for (const auto& user : {alice, bob, charlie, server_owner, server_account}) {
            for (const auto& host : {host_.value(), std::string("localhost")}) {
                passwords += user + " " + host + " " + std::to_string(port_.value()) + " " + password + "\n";
            }
        }
        // A plug forwards its user to the destination as a regular user: both password files hold the same entries
        const auto passwords_file        = home_.path() / "ecf.passwd";
        const auto custom_passwords_file = home_.path() / "ecf.custom_passwd";
        write_file_of(passwords_file, passwords, launch.account, 0600);
        write_file_of(custom_passwords_file, passwords, launch.account, 0600);

        const fs::path server = launch.executable.empty() ? fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server"
                                                          : launch.executable;

        std::vector<std::string> server_args{"--port", std::to_string(port_.value())};
        if (launch.owner) {
            server_args.emplace_back("--ownership_owner");
            server_args.emplace_back(*launch.owner);
        }

        // The server inherits the environment of the test, without any of its own settings but the ones set here
        std::list<ecf::test::scaffold::WithoutTestEnvironmentVariable> cleared;
        for (const auto* name : {"ECF_PORT",
                                 "ECF_HOST",
                                 "ECF_LOG",
                                 "ECF_CHECK",
                                 "ECF_CHECKOLD",
                                 "ECF_CHECKINTERVAL",
                                 "ECF_LISTS",
                                 "ECF_PRUNE_NODE_LOG",
                                 "ECF_TASK_THRESHOLD",
                                 "ECF_SSL",
                                 "ECF_DEBUG_SERVER",
                                 "ECF_USER",
                                 "ECF_PASS",
                                 "ECF_PASSWORD",
                                 "ECF_OWNERSHIP_OWNER",
                                 "ECF_OWNERSHIP_PATH",
                                 "ECF_OWNERSHIP_ENVIRONMENT"}) {
            cleared.emplace_back(name);
        }
        ecf::test::scaffold::WithTestEnvironmentVariable passwd("ECF_PASSWD", passwords_file.string());
        ecf::test::scaffold::WithTestEnvironmentVariable custom_passwd("ECF_CUSTOM_PASSWD",
                                                                       custom_passwords_file.string());
        ecf::test::scaffold::WithTestEnvironmentVariable home("ECF_HOME", home_.path().string());

        if (launch.account == "root") {
            process_ = ecf::test::scaffold::Process(server, server_args, home_.path());
        }
        else {
            std::vector<std::string> args{"--reuid=" + launch.account, "--regid=" + launch.account, "--init-groups"};
            if (launch.ambient_capabilities) {
                args.emplace_back("--inh-caps=+setuid,+setgid");
                args.emplace_back("--ambient-caps=+setuid,+setgid");
            }
            args.emplace_back("--");
            args.emplace_back(server.string());
            args.insert(args.end(), server_args.begin(), server_args.end());
            process_ = ecf::test::scaffold::Process(required_executable("setpriv"), args, home_.path());
        }
    }

    LaunchedServer(const LaunchedServer&)            = delete;
    LaunchedServer& operator=(const LaunchedServer&) = delete;

    ~LaunchedServer() { stop(); }

    ///
    /// @brief Waits, with a bound, until the server answers a ping.
    ///
    /// @return True when the server answers; false when it exited, or did not answer in time.
    ///
    bool wait_until_ready() {
        const auto deadline = std::chrono::steady_clock::now() + ecf::test::scaffold::server_startup_timeout;
        do {
            if (!process_.is_running() || has_exited()) {
                return false;
            }
            ClientInvoker pinger;
            configure(pinger, alice);
            if (pinger.pingServer() == 0) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (std::chrono::steady_clock::now() < deadline);
        return false;
    }

    ///
    /// @brief Waits until the server answers a ping, failing the test (with the output of the server) otherwise.
    ///
    /// The output is only collected on failure: collecting it stops the server.
    ///
    void require_ready() {
        if (!wait_until_ready()) {
            const auto output = stop();
            BOOST_FAIL("The server did not start; output:\n" << output);
        }
    }

    ///
    /// @brief Waits, with a bound, for the server to exit by itself, as it does when it refuses to start.
    ///
    /// @return The exit status, or no value when the server was still running at the end of the wait (it is then
    ///         killed).
    ///
    std::optional<int> wait_for_exit() {
        auto completion = std::async(std::launch::async, [this] { return process_.wait(); });
        if (completion.wait_for(ecf::test::scaffold::server_startup_timeout) != std::future_status::ready) {
            process_.terminate();
            static_cast<void>(completion.get());
            return std::nullopt;
        }
        return completion.get();
    }

    ///
    /// @brief Kills the server, if still running, and returns its output.
    ///
    /// @return The standard output followed by the standard error of the server.
    ///
    std::string stop() {
        if (process_.is_running()) {
            process_.terminate();
        }
        if (output_.empty() && !process_.is_running()) {
            try {
                output_ = process_.read_stdout() + process_.read_stderr();
            }
            catch (const std::exception&) {
                // No child (never launched)
            }
        }
        return output_;
    }

    ///
    /// @brief Configures a client for the server, authenticated as the given ecFlow user.
    ///
    /// @param[in,out] c The client, then reporting errors through return values
    /// @param[in] user The ecFlow user
    ///
    void configure(ClientInvoker& c, const std::string& user) const {
        c.set_host_port(host_.value(), std::to_string(port_.value()));
        c.set_throw_on_error(false);
        c.set_connection_attempts(1);
        c.set_connect_timeout(std::chrono::seconds(5));
        c.set_user_name(user);
        c.set_password(PasswordEncryption::encrypt(password, user));
    }

    ///
    /// @brief Returns the content of the log file of the server.
    ///
    /// @return The log, or an empty string when the server has not written it.
    ///
    [[nodiscard]] std::string log() const {
        std::ifstream input(home_.path() / (host_.value() + "." + port() + ".ecf.log"));
        return std::string{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    [[nodiscard]] const fs::path& home() const { return home_.path(); }
    [[nodiscard]] const std::string& host() const { return host_.value(); }
    [[nodiscard]] std::string port() const { return std::to_string(port_.value()); }

private:
    /// Whether the child has exited, although not yet reaped (a zombie), as seen in /proc (Linux)
    [[nodiscard]] bool has_exited() const {
        std::ifstream stat("/proc/" + std::to_string(process_.pid()) + "/stat");
        std::string content{std::istreambuf_iterator<char>(stat), std::istreambuf_iterator<char>()};
        const auto end_of_name = content.rfind(')');
        return !stat.is_open() || (end_of_name != std::string::npos && end_of_name + 2 < content.size() &&
                                   content[end_of_name + 2] == 'Z');
    }

    /// Reserves a port in the range assigned to the tests, from ECF_TEST_OWNERSHIP_PORT_BASE (1000 ports)
    static ecf::test::scaffold::Port reserve_port() {
        const char* base = std::getenv("ECF_TEST_OWNERSHIP_PORT_BASE");
        if (base == nullptr) {
            throw std::runtime_error(
                "ECF_TEST_OWNERSHIP_PORT_BASE is not defined (it is set by the CMake test definition)");
        }
        const auto first = static_cast<ecf::test::scaffold::Port::port_t>(std::stoul(base));
        for (auto candidate = first; candidate < first + 1000; ++candidate) {
            if (auto found = ecf::test::scaffold::SpecificPortValue::attempt_to_lock_port(candidate); found) {
                return ecf::test::scaffold::Port{found->first, std::move(found->second)};
            }
        }
        throw std::runtime_error("No free port in the range " + std::to_string(first) + "-" +
                                 std::to_string(first + 999));
    }

    AccountDirectory home_;
    ecf::test::scaffold::Host host_;
    ecf::test::scaffold::Port port_;
    ecf::test::scaffold::Process process_;
    std::string output_;
};

///
/// @brief Runs a command, and returns its exit status.
///
/// @param[in] executable The executable
/// @param[in] args The arguments, without the program name
/// @return The exit status and the output of the command.
///
inline std::pair<int, std::string> run(const fs::path& executable, const std::vector<std::string>& args) {
    ecf::test::scaffold::Process process(executable, args);
    const int status = process.wait();
    return {status, process.read_stdout() + process.read_stderr()};
}

} // namespace ecf::test::ownership
