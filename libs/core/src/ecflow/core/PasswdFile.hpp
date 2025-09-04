/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_PasswdFile_HPP
#define ecflow_core_PasswdFile_HPP

///
/// \brief Parser for password file
///

#include <string>
#include <vector>

// ----------------------------------------------------------------
//
class Pass_wd {
public:
    Pass_wd(const std::string& user, const std::string& host, const std::string& port, const std::string& passwd)
        : user_(user),
          host_(host),
          port_(port),
          passwd_(passwd) {}
    ~Pass_wd() = default;

    bool operator==(const Pass_wd& rhs) const {
        return rhs.user_ == user_ && rhs.host_ == host_ && rhs.port_ == port_ && rhs.passwd_ == passwd_;
    }
    const std::string& user() const { return user_; }
    const std::string& host() const { return host_; }
    const std::string& port() const { return port_; }
    const std::string& passwd() const { return passwd_; }

private:
    std::string user_;
    std::string host_;
    std::string port_;
    std::string passwd_; // always stored as ciphertext
};

// This class is used to authenticate, user commands, i.e. like ping,alter, etc
class PasswdFile {
public:
    PasswdFile();
    // Disable copy (and move) semantics
    PasswdFile(const PasswdFile&)                  = delete;
    const PasswdFile& operator=(const PasswdFile&) = delete;

    ~PasswdFile();

    [[nodiscard]] bool empty() const { return vec_.empty(); }

    // Parse the file if any errors found return false and errorMsg
    // The parser expects version number  4.5.0
    bool load(const std::string& file, bool debug, std::string& errorMsg);

    // to be called by the server, to at least one user with given host and port
    bool check_at_least_one_user_with_host_and_port(const std::string& host, const std::string& port);

    // get the password for the given user, host and port. Otherwise, return an empty string
    std::string get_passwd(const std::string& user, const std::string& host, const std::string& port);

    // authenticate the user, given the password.
    bool authenticate(const std::string& user, const std::string& passwd) const;

    // ===========================================================================
    // For test/debug

    // Will overwrite the existing file
    static bool createWithAccess(const std::string& pathToFile,
                                 const std::string& host,
                                 const std::string& port,
                                 const std::string& passwd,
                                 std::string& errorMsg);

    // Open password file and make it empty
    static bool clear(const std::string& pathToFile, std::string& errorMsg);

    const std::vector<Pass_wd>& passwds() const { return vec_; }
    std::string dump() const;

private:
    bool add_user(std::vector<std::string>& tokens, std::string& error_msg);
    bool validateVersionNumber(const std::string& line, std::string& errorMsg) const;

private:
    std::string passwd_file_;
    std::vector<Pass_wd> vec_;
};

#endif /* ecflow_core_PasswdFile_HPP */
