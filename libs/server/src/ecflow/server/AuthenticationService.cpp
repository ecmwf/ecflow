// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/AuthenticationService.hpp"

#include <iostream>
#include <map>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Filesystem.hpp"

namespace ecf {

void AuthenticationService::init(const Host& host, const std::string& port) {
    if (ecf_passwd_file_ == Ecf::PASSWD_FILE()) {
        ecf_passwd_file_ = host.prefix_host_and_port(port, ecf_passwd_file_);
    }

    if (ecf_passwd_custom_file_ == Ecf::CUSTOM_PASSWD_FILE()) {
        ecf_passwd_custom_file_ = host.prefix_host_and_port(port, ecf_passwd_custom_file_);
    }
}

namespace {

///
/// @brief Loads a present regular password file and checks for an identity matching this server.
///
/// @param[in] kind Human-readable password-file kind for diagnostics
/// @param[in] passwd_file Selected path; an absent file leaves authentication unconfigured
/// @param[in] debug_ Whether password-parser debug output is enabled
/// @param[out] passwd_db Password database populated from the selected file
/// @param[out] loaded Receives whether the file was loaded
/// @param[in] host Server hostname required in at least one entry
/// @param[in] port Server port required in at least one entry
/// @param[out] error Failure diagnostic containing the selected path and cause
/// @return True when the file is absent or contains valid credentials for this server.
/// @throws std::filesystem::filesystem_error if filesystem inspection fails.
///
bool valid_passwd(std::string_view kind,
                  const std::string& passwd_file,
                  bool debug_,
                  PasswdFile& passwd_db,
                  bool& loaded,
                  const std::string& host,
                  const std::string& port,
                  std::string& error) {
    loaded = false;
    // The outcome is reported by the authentication block of the log, and a failure by the error
    if (passwd_file.empty() || !fs::exists(passwd_file)) {
        return true;
    }

    if (!fs::is_regular_file(passwd_file)) {
        error = std::string(kind) + " file '" + passwd_file + "' is not a regular file";
        return false;
    }

    std::string parse_error;
    if (!passwd_db.load(passwd_file, debug_, parse_error)) {
        error = std::string(kind) + " file '" + passwd_file + "' could not be parsed: " + parse_error;
        return false;
    }
    loaded = true;

    if (!passwd_db.check_at_least_one_user_with_host_and_port(host, port)) {
        error = std::string(kind) + " file '" + passwd_file +
                "' does not contain any users with access on this server (" + host + ":" + port + ")";
        return false;
    }

    return true;
}

} // namespace

bool AuthenticationService::valid(const std::string& host, const std::string& port, std::string& error) const {
    bool loaded = valid_passwd("Password", ecf_passwd_file_, debug_, passwd_file_, passwd_loaded_, host, port, error);
    bool loaded_custom = valid_passwd("Custom Password",
                                      ecf_passwd_custom_file_,
                                      debug_,
                                      passwd_custom_file_,
                                      passwd_custom_loaded_,
                                      host,
                                      port,
                                      error);

    return loaded && loaded_custom;
}

bool AuthenticationService::is_authentic(const Identity& identity) const {
    if (identity.is_secure()) {
        return true;
    }

    const auto& username = identity.username();
    const auto& password = identity.password();

    if (!identity.is_custom()) {
        if (!passwd_file_.authenticate(username.value(), password.value())) {
            return false;
        }
    }
    else {
        if (!passwd_custom_file_.authenticate(username.value(), password.value())) {
            return false;
        }
    }
    return true;
}

bool AuthenticationService::reload_passwd_file(std::string& error) {
    if (debug_) {
        std::cout << "AuthenticationService::reload_passwd_file:(" << ecf_passwd_file_ << ") CWD("
                  << fs::current_path().string() << ")\n";
    }
    if (ecf_passwd_file_.empty()) {
        error += "The ECF_PASSWD file ";
        error += ecf_passwd_file_;
        error += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_passwd_file_)) {
        error += "The ECF_PASSWD file ";
        error += ecf_passwd_file_;
        error += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    if (!fs::is_regular_file(ecf_passwd_file_)) {
        error += "ECF_PASSWD file '" + ecf_passwd_file_ + "' is not a regular file";
        return false;
    }

    PasswdFile replacement;
    std::string parse_error;
    const bool loaded = replacement.load(ecf_passwd_file_, debug_, parse_error);
    if (loaded) {
        passwd_file_.swap(replacement);
        passwd_loaded_ = true;
    }
    else {
        error += "Could not reload ECF_PASSWD file '" + ecf_passwd_file_ + "': " + parse_error;
    }
    return loaded;
}

bool AuthenticationService::reload_custom_passwd_file(std::string& error) {
    if (debug_) {
        std::cout << "AuthenticationService::reload_custom_passwd_file:(" << ecf_passwd_custom_file_ << ") CWD("
                  << fs::current_path().string() << ")\n";
    }
    if (ecf_passwd_custom_file_.empty()) {
        error += "The ECF_CUSTOM_PASSWD file ";
        error += ecf_passwd_custom_file_;
        error += " has not been specified.";
        return false;
    }
    if (!fs::exists(ecf_passwd_custom_file_)) {
        error += "The ECF_CUSTOM_PASSWD file ";
        error += ecf_passwd_custom_file_;
        error += " does not exist. Server CWD : " + fs::current_path().string();
        return false;
    }

    if (!fs::is_regular_file(ecf_passwd_custom_file_)) {
        error += "ECF_CUSTOM_PASSWD file '" + ecf_passwd_custom_file_ + "' is not a regular file";
        return false;
    }

    PasswdFile replacement;
    std::string parse_error;
    const bool loaded = replacement.load(ecf_passwd_custom_file_, debug_, parse_error);
    if (loaded) {
        passwd_custom_file_.swap(replacement);
        passwd_custom_loaded_ = true;
    }
    else {
        error += "Could not reload ECF_CUSTOM_PASSWD file '" + ecf_passwd_custom_file_ + "': " + parse_error;
    }
    return loaded;
}

std::vector<std::string> AuthenticationService::describe() const {
    std::vector<std::string> lines;
    std::map<std::string, std::vector<std::string>> users; // user, the files listing it
    auto describe_file = [&](const char* name, const std::string& file, bool loaded, const PasswdFile& passwds) {
        if (file.empty()) {
            lines.push_back(std::string("  ") + name + " = not set");
            return;
        }
        lines.push_back(std::string("  ") + name + " (" + fs::absolute(file).lexically_normal().string() +
                        ") = " + (loaded ? "loaded" : "absent"));
        if (loaded) {
            for (const auto& passwd : passwds.passwds()) {
                auto& files = users[passwd.user()];
                if (files.empty() || files.back() != name) {
                    files.emplace_back(name);
                }
            }
        }
    };
    describe_file("ECF_PASSWD", ecf_passwd_file_, passwd_loaded_, passwd_file_);
    describe_file("ECF_CUSTOM_PASSWD", ecf_passwd_custom_file_, passwd_custom_loaded_, passwd_custom_file_);

    if (!users.empty()) {
        lines.emplace_back("  Users:");
        for (const auto& [user, files] : users) {
            std::string line = "    " + user + " =";
            for (size_t i = 0; i < files.size(); ++i) {
                line += (i == 0 ? " " : ", ") + files[i];
            }
            lines.push_back(line);
        }
    }
    return lines;
}

} // namespace ecf
