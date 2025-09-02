/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_AuthenticationService_HPP
#define ecflow_server_AuthenticationService_HPP

#include <string>

#include "ecflow/base/Identity.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/PasswdFile.hpp"

namespace ecf {

class AuthenticationService {
public:
    AuthenticationService() = default;

    void init(const Host& host, const std::string& port);
    void set_debug(bool debug) { debug_ = debug; }

    [[nodiscard]] bool valid(const std::string& host, const std::string& port, std::string& error) const;

    [[nodiscard]] bool is_authentic(const Identity& identity) const;

    [[nodiscard]] const std::string& passwd_file() const { return ecf_passwd_file_; }
    [[nodiscard]] const std::string& custom_passwd_file() const { return ecf_passwd_custom_file_; }

    void set_passwd_file(const std::string& passwd_file) { ecf_passwd_file_ = passwd_file; }
    void set_custom_passwd_file(const std::string& custom_passwd_file) { ecf_passwd_custom_file_ = custom_passwd_file; }

    void retrieve_passwd_file();
    void retrieve_custom_passwd_file();

    bool reload_passwd_file(std::string& error);
    bool reload_custom_passwd_file(std::string& error);

private:
    bool debug_{false};

    std::string ecf_passwd_file_;
    std::string ecf_passwd_custom_file_;

    mutable PasswdFile passwd_file_;
    mutable PasswdFile passwd_custom_file_;
};

} // namespace ecf

#endif /* ecflow_server_AuthenticationService_HPP */
