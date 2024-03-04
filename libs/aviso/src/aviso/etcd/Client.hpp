/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_etcd_Client_HPP
#define aviso_etcd_Client_HPP

#include <memory>
#include <string>
#include <vector>

#include "aviso/etcd/Address.hpp"

namespace aviso::etcd {

class Client {
public:
    Client() = default;
    Client(const Address& address);

    Client(const Client&) = delete;
    Client(Client&&)      = delete;

    ~Client();

    std::vector<std::pair<std::string, std::string>> poll(std::string_view key_prefix, int64_t revision);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    inline static const std::string endpoint_path = "/v3/kv/range";
};

} // namespace aviso::etcd

#endif /* aviso_Aviso_HPP */
