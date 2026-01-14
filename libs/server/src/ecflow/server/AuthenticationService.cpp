/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/AuthenticationService.hpp"

#include <iostream>

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Filesystem.hpp"

namespace ecf {

void AuthenticationService::init(const Host& host, const std::string& port) {
    if (ecf_passwd_file_ == default_passwd_file()) {
        ecf_passwd_file_ = host.prefix_host_and_port(port, ecf_passwd_file_);
    }

    if (ecf_passwd_custom_file_ == default_custom_passwd_file()) {
        ecf_passwd_custom_file_ = host.prefix_host_and_port(port, ecf_passwd_custom_file_);
    }
}

namespace {

bool valid_passwd(std::string_view kind,
                  const std::string& passwd_file,
                  bool debug_,
                  PasswdFile& passwd_db,
                  const std::string& host,
                  const std::string& port,
                  std::string& error) {
    if (passwd_file.empty()) {
        std::cout << "*** " << kind << " file not loaded: no file specified\n";
        return true;
    }

    if (!fs::exists(passwd_file)) {
        std::cout << "*** " << kind << " file not loaded: file '" << passwd_file << "' does not exist\n";
        return true;
    }

    std::cout << "*** " << kind << " file located at '" << passwd_file << "'\n";
    if (passwd_db.load(passwd_file, debug_, error)) {
        std::cout << "*** " << kind << " file successfully loaded\n";
    }
    else {
        std::cout << "!!! " << kind << " file '" << passwd_file << "' could not be parsed:\n" << error << "\n";
        return false;
    }

    if (!passwd_db.check_at_least_one_user_with_host_and_port(host, port)) {
        std::cout << "!!! " << kind << " file '" << passwd_file
                  << "' does not contain any users with access on this server\n";
        return false;
    }

    return true;
}

} // namespace

bool AuthenticationService::valid(const std::string& host, const std::string& port, std::string& error) const {
    bool loaded = valid_passwd("Password", ecf_passwd_file_, debug_, passwd_file_, host, port, error);
    bool loaded_custom =
        valid_passwd("Custom Password", ecf_passwd_custom_file_, debug_, passwd_custom_file_, host, port, error);

    return loaded && loaded_custom;
}

bool AuthenticationService::is_authentic(const Identity& identity) const {
    if (identity.is_secure()) {
        return true;
    }

    const auto& username = identity.username();
    const auto& password = identity.password();

    if (!identity.is_custom()) {
        if (!passwd_file_.authenticate(username, password)) {
            return false;
        }
    }
    else {
        if (!passwd_custom_file_.authenticate(username, password)) {
            return false;
        }
    }
    return true;
}

void AuthenticationService::retrieve_passwd_file() {
    ecf::environment::get(ecf::environment::ECF_PASSWD, ecf_passwd_file_);
}

void AuthenticationService::retrieve_custom_passwd_file() {
    ecf::environment::get(ecf::environment::ECF_CUSTOM_PASSWD, ecf_passwd_custom_file_);
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

    // Only override valid users if we successfully opened and parsed file
    std::cout << "*** Password file to be loaded from '" << ecf_passwd_file_ << "'\n";
    auto loaded = passwd_file_.load(ecf_passwd_file_, debug_, error);
    if (loaded) {
        std::cout << "*** Password file loaded [OK]\n";
    }
    else {
        std::cout << "*** Password file loaded [FAIL]: due to " << error << "\n";
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

    // Only override valid users if we successfully opened and parsed file
    auto loaded = passwd_custom_file_.load(ecf_passwd_custom_file_, debug_, error);
    if (loaded) {
        std::cout << "Password file " << ecf_passwd_file_ << " opening...\n";
    }
    return loaded;
}

} // namespace ecf
