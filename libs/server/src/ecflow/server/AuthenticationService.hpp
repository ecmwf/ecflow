/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_server_AuthenticationService_HPP
#define ecflow_server_AuthenticationService_HPP

#include <string>
#include <string_view>

#include "ecflow/core/Host.hpp"
#include "ecflow/core/Identity.hpp"
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

    static inline std::string_view default_passwd_file() { return "ecf.passwd"; }
    static inline std::string_view default_custom_passwd_file() { return "ecf.custom_passwd"; }

private:
    bool debug_{false};

    std::string ecf_passwd_file_;
    std::string ecf_passwd_custom_file_;

    mutable PasswdFile passwd_file_;
    mutable PasswdFile passwd_custom_file_;
};

} // namespace ecf

#endif /* ecflow_server_AuthenticationService_HPP */
