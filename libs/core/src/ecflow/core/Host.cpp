/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Host.hpp"

#include <cassert>
#include <stdexcept>
#include <unistd.h> // for gethostname

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;

namespace ecf {

Host::Host() {
    get_host_name();
}

Host::Host(const std::string& host) : the_host_name_(host) {
    if (the_host_name_ == Str::LOCALHOST()) {
        get_host_name();
    }
}

void Host::get_host_name() {
    static std::string the_host_name;
    if (the_host_name.empty()) {
        char hostNameArray[255];
        if (gethostname(hostNameArray, 255) != -1) {
            the_host_name = string(hostNameArray);
        }
        else {
            throw std::runtime_error("Host::Host() failed, could not get host name?\n");
        }
    }
    the_host_name_ = the_host_name;
    assert(!the_host_name_.empty());
}

std::string Host::name() const {
    return the_host_name_;
}

std::string Host::ecf_log_file(const std::string& port) const {
    return prefix_host_and_port(port, Ecf::LOG_FILE());
}

std::string Host::ecf_checkpt_file(const std::string& port) const {
    return prefix_host_and_port(port, Ecf::CHECKPT());
}

std::string Host::ecf_backup_checkpt_file(const std::string& port) const {
    return prefix_host_and_port(port, Ecf::BACKUP_CHECKPT());
}

std::string Host::ecf_lists_file(const std::string& port) const {
    return prefix_host_and_port(port, Str::WHITE_LIST_FILE());
}

std::string Host::ecf_passwd_file(const std::string& port) const {
    return prefix_host_and_port(port, ecf::environment::ECF_PASSWD);
}

std::string Host::ecf_custom_passwd_file(const std::string& port) const {
    return prefix_host_and_port(port, ecf::environment::ECF_CUSTOM_PASSWD);
}

std::string Host::prefix_host_and_port(const std::string& port, const std::string& file_name) const {
    // The file name may include a path.  /user/avi/fred.log
    //    fred.log             ->  <host>.<port>.fred.log
    //    /user/avi/fred.log   ->  /user/avi/fred.log
    if (!file_name.empty() && file_name.find("/") != std::string::npos) {
        return file_name;
    }
    std::string ret = host_port_prefix(port);
    ret += ".";
    ret += file_name;
    return ret;
}

std::string Host::host_port_prefix(const std::string& port) const {
    std::string ret = the_host_name_;
    if (!port.empty()) {
        ret += ".";
        ret += port;
    }
    return ret;
}

} // namespace ecf
