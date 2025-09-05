/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/PasswdFile.hpp"

#include <iostream>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/User.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

PasswdFile::PasswdFile()  = default;
PasswdFile::~PasswdFile() = default;

// #define DEBUG_ME 1

// Parse the file if any errors found return false and errorMsg
// The parser expects version number  4.5.0
bool PasswdFile::load(const std::string& file, bool debug, std::string& errorMsg) {
#ifdef DEBUG_ME
    debug = true;
#endif
    vec_.clear();
    passwd_file_ = file;
    if (debug) {
        std::cout << __func__ << "  " << passwd_file_ << " opening...\n";
    }

    std::vector<std::string> lines;
    if (!File::splitFileIntoLines(passwd_file_, lines, true /* ignore empty lines */)) {
        errorMsg += "Could not open file specified by ECF_PASSWD/ECF_CUSTOM_PASSWD ";
        errorMsg += passwd_file_;
        errorMsg += " (";
        errorMsg += strerror(errno);
        errorMsg += ")";
        if (debug) {
            std::cout << dump() << "\n";
        }
        return false;
    }

    bool foundVersionNumber = false; // can read from version 4.4.0 onwards
    size_t lines_size       = lines.size();
    for (size_t i = 0; i < lines_size; ++i) {

        if (lines[i].empty()) {
            continue;
        }

        // ignore/remove all comments
        if (lines[i][0] == '#') {
            continue;
        }
        std::string theLine           = lines[i];
        string::size_type comment_pos = theLine.find("#");
        if (comment_pos != std::string::npos) {
            theLine.erase(comment_pos);
        }

        ecf::algorithm::trim(theLine); // remove leading and trailing spaces
        std::vector<std::string> lineTokens;
        Str::split(theLine, lineTokens);
        if (lineTokens.empty()) {
            continue;
        }

        // version should be at the start
        if (!foundVersionNumber) {

            if (!validateVersionNumber(lineTokens[0], errorMsg)) {
                std::stringstream ss;
                ss << " " << i + 1 << ": " << lines[i] << "\n";
                ss << "for ECF_PASSWD/ECF_CUSTOM_PASSWD file " << passwd_file_ << "\n";
                errorMsg += ss.str();
                return false;
            }
            foundVersionNumber = true;
            continue;
        }
        else {
            if (!add_user(lineTokens, errorMsg)) {
                errorMsg += theLine;
                return false;
            }
        }
    }

    if (debug) {
        std::cout << dump();
    }

    // Now check the user is unique for a given host/port:
    //   fred host 3141 xxxx
    //   fred host 3141 yyyy  this is illegal.
    size_t vec_size = vec_.size();
    for (size_t i = 0; i < vec_size; i++) {
        for (size_t k = 0; k < vec_size; k++) {
            if (i == k) {
                continue;
            }
            if (vec_[i].user() == vec_[k].user() && vec_[i].host() == vec_[k].host() &&
                vec_[i].port() == vec_[k].port()) {
                std::stringstream ss;
                ss << "user " << vec_[i].user() << " can only appear once for given host/port\n";
                errorMsg += ss.str();
                return false;
            }
        }
    }
    return true;
}

bool PasswdFile::check_at_least_one_user_with_host_and_port(const std::string& host, const std::string& port) {
    size_t vec_size = vec_.size();
    for (size_t i = 0; i < vec_size; i++) {
        if (vec_[i].host() == host && vec_[i].port() == port) {
            return true;
        }
    }
    return false;
}

std::string PasswdFile::get_passwd(const std::string& user, const std::string& host, const std::string& port) {
#ifdef DEBUG_ME
    cout << "PasswdFile::get_passwd user(" << user << ") host(" << host << ") port(" << port << ")\n";
#endif
    size_t vec_size = vec_.size();
    for (size_t i = 0; i < vec_size; i++) {
        if (vec_[i].user() == user && vec_[i].host() == host && vec_[i].port() == port) {
            return vec_[i].passwd();
        }
    }
    return string();
}

bool PasswdFile::authenticate(const std::string& user, const std::string& passwd) const {
#ifdef DEBUG_ME
    cout << "   PasswdFile::authenticate user:" << user << " passwd:" << passwd << " file:" << passwd_file_
         << " vec_.size(): " << vec_.size() << "\n";
    cout << dump() << "\n";
#endif

    if (user.empty()) {
#ifdef DEBUG_ME
        cout << "      PasswdFile::authenticate: user empty:  FAIL\n";
#endif
        return false;
    }

    // no password specified, and password file is empty
    if (passwd.empty() && vec_.empty()) {
#ifdef DEBUG_ME
        cout << "      PasswdFile::authenticate no password and password file empty: PASS\n";
#endif
        return true;
    }

    // Only fail if user exists in the passwd file
    size_t vec_size = vec_.size();
    for (size_t i = 0; i < vec_size; i++) {
        if (vec_[i].user() == user) {
            if (vec_[i].passwd() == passwd) {
#ifdef DEBUG_ME
                cout << "      PasswdFile::authenticate user and password match: PASS\n";
#endif
                return true;
            }
            else {
#ifdef DEBUG_ME
                cout << "      PasswdFile::authenticate: user found but passwd did not match: FAIL\n";
#endif
                return false;
            }
        }
    }

    // Will ONLY reach here if user not found

    // User not found, but if passwd is not empty, then fail.
    if (!passwd.empty()) {
#ifdef DEBUG_ME
        cout << "      PasswdFile::authenticate: user NOT found, passwd is NOT EMPTY: FAIL\n";
#endif
        return false;
    }

    // Server has a password file, but user not found and passwd is empty.
    if (vec_.empty()) {
#ifdef DEBUG_ME
        cout << "      PasswdFile::authenticate: true, user NOT found, and passwd EMPTY, and password file EMPTY\n";
#endif
        return true;
    }

#ifdef DEBUG_ME
    cout << "      PasswdFile::authenticate false, user NOT found, and passwd EMPTY\n";
#endif
    return false;
}

bool PasswdFile::validateVersionNumber(const std::string& line, std::string& errorMsg) const {
    // Expect 4.5.0
    // If first character is NUMERIC and we have dots
    bool firstCharIsNumeric = Str::NUMERIC().find(line[0], 0) != string::npos;
    if (firstCharIsNumeric && line.find(".") != string::npos) {

        std::vector<std::string> versionNumberTokens;
        Str::split(line, versionNumberTokens, ".");
        if (versionNumberTokens.size() != 3) {
            std::stringstream ss;
            ss << "Expected version of the form <int>.<int>.<int> i.e 4.4.0. but found invalid version number\n";
            errorMsg += ss.str();
            return false;
        }

        try {
            auto major = ecf::convert_to<int>(versionNumberTokens[0]);
            auto minor = ecf::convert_to<int>(versionNumberTokens[1]);
            auto part  = ecf::convert_to<int>(versionNumberTokens[2]);
            if (major < 4) {
                errorMsg += "Only passwd files with a version >= 4.5.0 is supported\n";
                return false;
            }
            if (major == 4 && minor < 5) {
                errorMsg += "Only passwd files with a version >= 4.5.0 is supported\n";
                return false;
            }
            if (major == 4 && minor == 5 && part > 0) {
                errorMsg += "Only passwd files with a version >= 4.5.0 is supported\n";
                return false;
            }
        }
        catch (const ecf::bad_conversion&) {
            errorMsg += "Invalid version number \n";
            return false;
        }

        return true;
    }

    errorMsg += "The version number not found. The version number must appear before the users.\n";
    return false;
}

bool PasswdFile::add_user(std::vector<std::string>& tokens, std::string& error_msg) {
    // Expected format is:
    // 4.5.0
    // # comment
    // <user> <host> <port> <passwd> # comment
    // <user> <host> <port> <passwd> # comment

    if (tokens.size() < 4) {
        error_msg = "expected <user> <host> <port> <passwd> # comment\n";
        return false;
    }

    try {
        ecf::convert_to<int>(tokens[2]);
    }
    catch (const ecf::bad_conversion&) {
        error_msg += "Port number must be integer's\n";
        return false;
    }

    vec_.emplace_back(tokens[0], tokens[1], tokens[2], PasswordEncryption::encrypt(tokens[3], tokens[0]));

    return true;
}

std::string PasswdFile::dump() const {
    std::stringstream ss;
    int count = 1;
    for (const auto& i : vec_) {
        ss << count << ": " << i.user() << " " << i.host() << " " << i.port() << "\n";
        count++;
    }
    return ss.str();
}

bool PasswdFile::createWithAccess(const std::string& pathToFile,
                                  const std::string& host,
                                  const std::string& port,
                                  const std::string& passwd,
                                  std::string& errorMsg) {
    std::vector<std::string> lines;
    lines.reserve(3);

    lines.emplace_back("4.5.0");

    auto username = get_login_name();

    string line;
    line += username;
    line += " ";
    line += host;
    line += " ";
    line += port;
    line += " ";
    line += passwd;
    lines.push_back(line);

    line.clear();
    line += username;
    line += " ";
    line += Str::LOCALHOST();
    line += " ";
    line += port;
    line += " ";
    line += passwd;
    lines.push_back(line);

    return File::create(pathToFile, lines, errorMsg);
}

bool PasswdFile::clear(const std::string& pathToFile, std::string& errorMsg) {
    std::vector<std::string> lines;
    if (File::splitFileIntoLines(pathToFile, lines, true /* ignore empty lines */)) {
        // Just leave the version, i.e. the first line.
        if (lines.size() > 1) {
            lines.erase(lines.begin() + 1, lines.end());

            // Overwrite the file.
            return File::create(pathToFile, lines, errorMsg);
        }
        return true;
    }
    errorMsg += "PasswdFile::clear: Could not open file ";
    errorMsg += pathToFile;
    errorMsg += " (";
    errorMsg += strerror(errno);
    errorMsg += ")";
    return false;
}
