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
    if (ecf_passwd_file_ == ecf::environment::ECF_PASSWD) {
        ecf_passwd_file_ = host.prefix_host_and_port(port, ecf_passwd_file_);
    }

    if (ecf_passwd_custom_file_ == ecf::environment::ECF_CUSTOM_PASSWD) {
        ecf_passwd_custom_file_ = host.prefix_host_and_port(port, ecf_passwd_custom_file_);
    }
}

bool AuthenticationService::valid(const std::string& host, const std::string& port, std::string& error) const {
    if (!ecf_passwd_file_.empty() && fs::exists(ecf_passwd_file_)) {
        if (!passwd_file_.load(ecf_passwd_file_, debug_, error)) {
            std::cout << "Error: could not parse ECF_PASSWD file " << ecf_passwd_file_ << "\n" << error << "\n";
            return false;
        }
        if (!passwd_file_.check_at_least_one_user_with_host_and_port(host, port)) {
            std::cout << "Error: password file " << ecf_passwd_file_
                      << " does not contain any users, which match the host and port of this server\n";
            return false;
        }
    }

    if (!ecf_passwd_custom_file_.empty() && fs::exists(ecf_passwd_custom_file_)) {
        if (!passwd_custom_file_.load(ecf_passwd_custom_file_, debug_, error)) {
            std::cout << "Error: could not parse ECF_CUSTOM_PASSWD file " << ecf_passwd_custom_file_ << "\n"
                      << error << "\n";
            return false;
        }
        if (!passwd_custom_file_.check_at_least_one_user_with_host_and_port(host, port)) {
            std::cout << "Error: custom password file " << ecf_passwd_custom_file_
                      << " does not contain any users, which match the host and port of this server\n";
            return false;
        }
    }
    return true;
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
    return passwd_file_.load(ecf_passwd_file_, debug_, error);
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
    return passwd_custom_file_.load(ecf_passwd_custom_file_, debug_, error);
}

} // namespace ecf
