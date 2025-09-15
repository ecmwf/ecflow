/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_aviso_etcd_Client_HPP
#define ecflow_service_aviso_etcd_Client_HPP

#include <string>
#include <vector>

#include "ecflow/core/HttpLibrary.hpp"

namespace ecf::service::aviso::etcd {

class Client {
public:
    Client(const std::string& address);
    Client(const std::string& address, const std::string& auth_token);

    Client(const Client&) = delete;
    Client(Client&&)      = delete;

    ~Client();

    const std::string& address() const { return address_; }
    const std::string& auth_token() const { return auth_token_; }

    std::vector<std::pair<std::string, std::string>> poll(std::string_view key_prefix, int64_t revision);

private:
    httplib::Client client_;
    std::string address_;
    std::string auth_token_;

    inline static const std::string endpoint_path = "/v3/kv/range";
};

} // namespace ecf::service::aviso::etcd

#endif /* ecflow_service_aviso_etcd_Client_HPP */
