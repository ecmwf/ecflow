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

///
/// @brief Represents a host and provides helpers to build ecFlow server file names
///        prefixed with the host and port (e.g. @c <host>.<port>.ecf.log).
///
class Host {
public:
    ///
    /// @brief Create a Host instance by resolving the current machine's hostname.
    ///
    /// @throws std::runtime_error if the hostname cannot be determined
    ///
    Host();

    ///
    /// @brief Create a Host instance with the given hostname.
    ///
    /// If @p host is @c "localhost", the actual machine hostname is resolved.
    ///
    /// @param host the hostname to use
    /// @throws std::runtime_error if @p host is @c "localhost" and the hostname cannot be determined
    ///
    explicit Host(const std::string& host);

    // Disable copy (and move) semantics
    Host(const Host&)            = delete;
    Host& operator=(const Host&) = delete;
    Host(Host&&)                 = delete;
    Host& operator=(Host&&)      = delete;

    ~Host() = default;

    ///
    /// @brief Get the hostname.
    ///
    /// @return the hostname
    ///
    std::string name() const;

    ///
    /// @brief Get the log file name prefixed with the host and port.
    ///
    /// @param port the server port
    /// @return the log file name of the form @c <host>.<port>.ecf.log
    ///
    std::string ecf_log_file(const std::string& port) const;

    ///
    /// @brief Get the checkpoint file name prefixed with the host and port.
    ///
    /// @param port the server port
    /// @return the checkpoint file name of the form @c <host>.<port>.check
    ///
    std::string ecf_checkpt_file(const std::string& port) const;

    ///
    /// @brief Get the backup checkpoint file name prefixed with the host and port.
    ///
    /// @param port the server port
    /// @return the backup checkpoint file name of the form @c <host>.<port>.check.b
    ///
    std::string ecf_backup_checkpt_file(const std::string& port) const;

    ///
    /// @brief Get the white-list file name prefixed with the host and port.
    ///
    /// The white-list file is used for authentication and authorisation.
    ///
    /// @param port the server port
    /// @return the white-list file name of the form @c <host>.<port>.ecf.lists
    ///
    std::string ecf_lists_file(const std::string& port) const;

    ///
    /// @brief Get the password file name prefixed with the host and port.
    ///
    /// The password file is used for authentication.
    ///
    /// @param port the server port
    /// @return the password file name of the form @c <host>.<port>.ecf.passwd
    ///
    std::string ecf_passwd_file(const std::string& port) const;

    ///
    /// @brief Get the custom password file name prefixed with the host and port.
    ///
    /// The custom password file is used for authentication.
    ///
    /// @param port the server port
    /// @return the custom password file name of the form @c <host>.<port>.ecf.custom_passwd
    ///
    std::string ecf_custom_passwd_file(const std::string& port) const;

    ///
    /// @brief Build a file name prefixed with the host and port.
    ///
    /// If @p file_name contains a path separator (@c /), it is returned unchanged.
    /// Otherwise, returns a name of the form @c <host>.<port>.<file_name>.
    ///
    /// @param port      the server port
    /// @param file_name the base file name or absolute path
    /// @return the prefixed file name, or @p file_name unchanged if it contains a path separator
    ///
    std::string prefix_host_and_port(const std::string& port, std::string_view file_name) const;

private:
    std::string host_port_prefix(const std::string& port) const;
    void get_host_name(); // will cache host name, to avoid multiple system calls
    std::string the_host_name_;
};

} // namespace ecf

#endif /* ecflow_core_Host_HPP */
