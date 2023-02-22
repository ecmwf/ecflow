/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_UDP_CLIENTAPI_HPP
#define ECFLOW_UDP_CLIENTAPI_HPP

#include <memory>

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
    ~ClientAPI();

    void set_authentication(const std::string& username, const std::string& password);

    void update_meter(const std::string& path, const std::string& name, const std::string& value) const;
    void update_label(const std::string& path, const std::string& name, const std::string& value) const;

    void clear_event(const std::string& path, const std::string& name) const;
    void set_event(const std::string& path, const std::string& name) const;

private:
    std::unique_ptr<ClientInvoker> invoker_;
};

} // namespace ecf

#endif
