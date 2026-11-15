// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <fstream>
#include <future>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "SCPort.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/EcfPortLock.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace ecf::test {

///
/// @brief Holds the user name and password of an identity used by a test.
///
struct Credentials
{
    std::string user;     ///< The user name
    std::string password; ///< The password, in plain text
};

///
/// @brief Encrypts a password as the password files hold it, with the user name as salt.
///
/// Some C libraries (e.g. libxcrypt) refuse a salt holding a character outside [./0-9A-Za-z]; for such a name the
/// password is returned unchanged, since the tests use these names only for requests refused before any password is
/// compared.
///
/// @param[in] password The password in clear.
/// @param[in] user The user name, used as salt.
/// @return The encrypted password, or the password itself when the C library cannot encrypt it.
///
inline std::string encrypted_password(const std::string& password, const std::string& user) {
    try {
        return PasswordEncryption::encrypt(password, user);
    }
    catch (const std::runtime_error&) {
        return password;
    }
}

///
/// @brief Runs a server configured for tests of Permissions attributes, and issues requests under several identities.
///
/// The server runs in a directory of its own, from a configuration file `<host>.<port>.ecf.cfg` holding the given
/// server Permissions attribute, if any. Every identity is listed, with its password, in both password files of the
/// server, which therefore authenticate no other user, the user running the test included (unless the server is
/// open to the login user). Every request, set-up and
/// tear-down included, is issued under explicit credentials.
///
/// The environment variables read by servers and clients are cleared for the lifetime of the object, before the port
/// is chosen. The port stays reserved until the server is stopped. The server is stopped when the object is destroyed,
/// and when its construction fails: terminated under the administrator's credentials, and killed when it does not
/// terminate within a bounded wait.
///
class PermissionsServer {
public:
    ///
    /// @brief Starts the server, and checks that the server answering on its port is the one started.
    ///
    /// @param[in] admin The identity used for set-up and tear-down; it is listed in the password files
    /// @param[in] users Further identities, listed in the password files
    /// @param[in] permissions The server Permissions attribute, as its JSON form; none when empty
    /// @param[in] timeout The time allowed for the server to answer
    /// @param[in] fixed_port A port to use instead of reserving one, for tests of the harness itself; the harness
    ///            neither locks nor releases it
    /// @param[in] executable An executable to launch instead of ecflow_server, for tests of the harness itself
    /// @param[in] open_to_login_user When true, the server has no password file for users that are not custom, so
    ///            that the user running the test is authenticated without a password, as the REST service requires
    /// @param[in] settings Further members of `/ecflow/server` in the configuration file, as JSON text, e.g.
    ///            `"whitelist": {"ignore": true}`; none when empty. With `"http": true`, the clients use HTTP.
    /// @throws std::runtime_error when the server does not answer within the timeout, or another server answers; the
    ///         process launched, if any, is killed (never sent a request) and the port released.
    /// @throws std::exception when the process cannot be launched, with the same cleanup.
    ///
    PermissionsServer(Credentials admin,
                      std::vector<Credentials> users,
                      const std::string& permissions,
                      std::chrono::seconds timeout                 = std::chrono::seconds(30),
                      const std::optional<std::string>& fixed_port = std::nullopt,
                      const std::optional<fs::path>& executable    = std::nullopt,
                      bool open_to_login_user                      = false,
                      const std::string& settings                  = {})
        : administrator_{std::move(admin)},
          users_{std::move(users)},
          cleared_{cleared_environment()},
          reservation_{fixed_port ? Reservation(*fixed_port, false) : Reservation(SCPort::next(), true)},
          directory_{scaffold::MakeDirectory{}.create()},
          http_{!settings.empty() && nlohmann::json::parse("{" + settings + "}").value("http", false)} {
        try {
            const auto host = Host{}.name();
            if (!open_to_login_user) {
                write_passwords(home() / Host{}.prefix_host_and_port(port(), Ecf::PASSWD_FILE()));
            }
            write_passwords(home() / Host{}.prefix_host_and_port(port(), Ecf::CUSTOM_PASSWD_FILE()));
            std::ofstream(directory_.path() / (host + "." + port() + ".ecf.cfg"))
                << R"({"version": 1, "ecflow": {"server": {)"
                << (permissions.empty() ? std::string{} : R"("permissions": )" + permissions + ", ")
                << (settings.empty() ? std::string{} : settings + ", ")
                << R"("variables": {"ECF_HOME": "home", "ECF_LOG": "server.log", "ECF_CHECKMODE": "CHECK_NEVER"}}}})";

            process_ = std::make_unique<scaffold::Process>(executable.value_or(fs::path(File::find_ecf_server_path())),
                                                           std::vector<std::string>{"--port=" + port()},
                                                           directory_.path());

            if (!administrator()->wait_for_server_reply(static_cast<int>(timeout.count()))) {
                throw std::runtime_error("PermissionsServer: the server did not answer on port " + port());
            }
            if (!answers_from_own_home()) {
                throw std::runtime_error("PermissionsServer: another server answers on port " + port());
            }
            owned_ = true;
        }
        catch (...) {
            // Until the server answering is known to be the one launched, nothing is sent to the port
            kill();
            throw;
        }
    }

    PermissionsServer(const PermissionsServer&)            = delete;
    PermissionsServer& operator=(const PermissionsServer&) = delete;

    ~PermissionsServer() { stop(); }

    ///
    /// @brief Returns a client issuing requests to the server under the given identity.
    ///
    /// @param[in] credentials The identity; it need not be listed, so that refused identities can be tested
    /// @return The client, which reports errors through its return values rather than exceptions.
    ///
    [[nodiscard]] std::unique_ptr<ClientInvoker> client(const Credentials& credentials) const {
        auto client = std::make_unique<ClientInvoker>(std::string(string_constants::localhost), port());
        client->set_throw_on_error(false);
        client->set_connection_attempts(1);
        if (http_) {
            client->enable_http();
        }
        client->set_user_name(credentials.user);
        // The client sends the password in the encrypted form held by the password files
        client->set_password(encrypted_password(credentials.password, credentials.user));
        return client;
    }

    ///
    /// @brief Returns a client issuing requests under the administrator's identity.
    ///
    /// @return The client, which reports errors through its return values rather than exceptions.
    ///
    [[nodiscard]] std::unique_ptr<ClientInvoker> administrator() const { return client(administrator_); }

    ///
    /// @brief Stops the server, if it runs, under the administrator's identity; see stop(const Credentials&).
    ///
    /// @return True when the server terminated on request.
    ///
    bool stop() { return stop(administrator_); }

    ///
    /// @brief Stops the server, if it runs: terminated under the given identity, else killed; then releases the port.
    ///
    /// @param[in] credentials The identity requesting the termination
    /// @return True when the server terminated on request; false when it was killed, or had already stopped.
    ///
    bool stop(const Credentials& credentials) {
        if (!owned_) {
            kill();
            return false;
        }
        bool terminated = false;
        if (process_ && process_->is_running()) {
            if (client(credentials)->terminateServer() == 0) {
                auto exit  = std::async(std::launch::async, [this] { return process_->wait(); });
                terminated = exit.wait_for(std::chrono::seconds(30)) == std::future_status::ready;
                if (!terminated) {
                    process_->terminate();
                }
                (void)exit.get();
            }
            else {
                process_->terminate();
            }
        }
        reservation_.release();
        return terminated;
    }

    ///
    /// @brief Kills the launched process, if any, without sending any request, and releases the port.
    ///
    void kill() {
        if (process_) {
            process_->terminate();
        }
        reservation_.release();
    }

    ///
    /// @brief Returns whether the server process runs.
    ///
    /// @return True until the server is stopped.
    ///
    [[nodiscard]] bool running() const { return process_ && process_->is_running(); }

    ///
    /// @brief Returns the process id of the server.
    ///
    /// @return The process id, or -1 when no process was launched.
    ///
    [[nodiscard]] int pid() const { return process_ ? process_->pid() : -1; }

    ///
    /// @brief Returns the port of the server.
    ///
    /// @return The port, which stays reserved until the server is stopped.
    ///
    [[nodiscard]] const std::string& port() const { return reservation_.port(); }

    ///
    /// @brief Returns the start-up directory of the server.
    ///
    /// @return The directory, removed with the object.
    ///
    [[nodiscard]] fs::path directory() const { return directory_.path(); }

    ///
    /// @brief Returns the home of the server, which holds its log and password files.
    ///
    /// @return The directory `home` of the start-up directory.
    ///
    [[nodiscard]] fs::path home() const { return directory_.path() / "home"; }

private:
    // Owns the lock of a port reserved by SCPort::next(), from its acquisition
    class Reservation {
    public:
        Reservation(std::string port, bool owned)
            : port_{std::move(port)},
              released_{!owned} {}
        Reservation(const Reservation&)            = delete;
        Reservation& operator=(const Reservation&) = delete;
        ~Reservation() { release(); }
        [[nodiscard]] const std::string& port() const { return port_; }
        void release() {
            if (!released_) {
                scaffold::EcfPortLock::remove(port_);
                released_ = true;
            }
        }

    private:
        std::string port_;
        bool released_{false};
    };

    static std::list<scaffold::WithoutTestEnvironmentVariable> cleared_environment() {
        std::list<scaffold::WithoutTestEnvironmentVariable> cleared;
        for (const auto* name : {"ECF_HOME",
                                 "ECF_PORT",
                                 "ECF_LOG",
                                 "ECF_CHECK",
                                 "ECF_CHECKOLD",
                                 "ECF_PASSWD",
                                 "ECF_CUSTOM_PASSWD",
                                 "ECF_LISTS",
                                 "ECF_SSL",
                                 "ECF_USER",
                                 "ECF_HOST",
                                 "ECF_HOST_PROTOCOL",
                                 "ECF_PASSWORD"}) {
            cleared.emplace_back(name);
        }
        return cleared;
    }

    // The server answering has the home of the server started; only the administrator is authenticated by it
    [[nodiscard]] bool answers_from_own_home() const {
        auto probe = administrator();
        if (probe->getDefs() != 0 || !probe->defs()) {
            return false;
        }
        return fs::equivalent(probe->defs()->server_state().find_variable("ECF_HOME"), home());
    }

    void write_passwords(const fs::path& path) const {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        file << "4.5.0\n";
        std::vector<const Credentials*> identities{&administrator_};
        for (const auto& user : users_) {
            identities.push_back(&user);
        }
        for (const auto* identity : identities) {
            for (const auto& host : {Host{}.name(), std::string(string_constants::localhost)}) {
                file << identity->user << " " << host << " " << port() << " " << identity->password << "\n";
            }
        }
    }

    Credentials administrator_;
    std::vector<Credentials> users_;
    std::list<scaffold::WithoutTestEnvironmentVariable> cleared_;
    Reservation reservation_;
    scaffold::Directory directory_;
    std::unique_ptr<scaffold::Process> process_;
    bool owned_{false}; // true once the server answering on the port is known to be the one launched
    bool http_{false};  // true when the server uses HTTP, and so do its clients
};

} // namespace ecf::test
