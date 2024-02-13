/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_ClientApi_HPP
#define ecflow_udp_ClientApi_HPP

#include <memory>
#include <stdexcept>
#include <string>

// Forward Declaration
class ClientInvoker;

namespace ecf {

struct ClientAPIException : public std::runtime_error
{
    explicit ClientAPIException(const char* msg) : std::runtime_error(msg) {}
    explicit ClientAPIException(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * The ClientAPI enables the operations available to ecFlow UDP clients.
 */
class ClientAPI {
public:
    ClientAPI();
    ClientAPI(const ClientAPI&) = delete;
    ClientAPI(ClientAPI&&)      = delete;
    ~ClientAPI();

    /// Define the User Name
    void user_set_name(const std::string& username);

    /// Define the User Password
    void user_set_password(const std::string& password);

    void user_update_meter(const std::string& path, const std::string& name, const std::string& value) const;
    void user_update_label(const std::string& path, const std::string& name, const std::string& value) const;
    void user_clear_event(const std::string& path, const std::string& name) const;
    void user_set_event(const std::string& path, const std::string& name) const;

    /// Define the Remote Id (i.e. ECF_RID) for a child Task request
    void child_set_remote_id(const std::string& pid);
    /// Define the Password (i.e. ECF_PASS) for a child Task request
    void child_set_password(const std::string& password);
    /// Define the Try Number (i.e. ECF_TRYNO) for a child Task request
    void child_set_try_no(int try_no);

    // Notice that, in the following functions, path represents ECF_NAME for a Task request
    void child_update_meter(const std::string& path, const std::string& name, const std::string& value) const;
    void child_update_label(const std::string& path, const std::string& name, const std::string& value) const;
    void child_clear_event(const std::string& path, const std::string& name) const;
    void child_set_event(const std::string& path, const std::string& name) const;

private:
    template <typename F>
    void try_invoke(F f) const;

private:
    std::unique_ptr<ClientInvoker> invoker_;
};

} // namespace ecf

#endif /* ecflow_udp_ClientApi_HPP */
