/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Host_HPP
#define ecflow_core_Host_HPP

#include <string>

namespace ecf {

class Host {
public:
    /// can throw std::runtime_error if the gethostname fails
    Host();
    explicit Host(const std::string& host);

    /// return the host name
    std::string name() const;

    /// returns the log file name
    std::string ecf_log_file(const std::string& port) const;

    /// return checkPoint file
    std::string ecf_checkpt_file(const std::string& port) const;

    /// return backup checkPoint file
    std::string ecf_backup_checkpt_file(const std::string& port) const;

    /// return ecf.list file. White list file used for authentication & authorisation
    std::string ecf_lists_file(const std::string& port) const;

    /// return ecf.passwd file. Used for authentication
    std::string ecf_passwd_file(const std::string& port) const;

    /// return ecf.custom_passwd file. Used for authentication
    std::string ecf_custom_passwd_file(const std::string& port) const;

    /// Given a port and file name, will return <host>.<port>.file_name
    std::string prefix_host_and_port(const std::string& port, const std::string& file_name) const;

private:
    Host(const Host&)                  = delete;
    const Host& operator=(const Host&) = delete;

private:
    std::string host_port_prefix(const std::string& port) const;
    void get_host_name(); // will cache host name, to avoid multiple sysm calls
    std::string the_host_name_;
};

} // namespace ecf

#endif /* ecflow_core_Host_HPP */
