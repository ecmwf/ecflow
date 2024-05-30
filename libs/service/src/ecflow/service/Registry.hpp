/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_Registry_HPP
#define ecflow_service_Registry_HPP

#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/service/Log.hpp"

namespace ecf::service {

class TheOneServer {
public:
    static void set_server(AbstractServer* server) { TheOneServer::instance().server_ = server; }
    static AbstractServer* server() { return TheOneServer::instance().server_; }

private:
    TheOneServer() = default;

    static TheOneServer& instance() {
        static TheOneServer the_one_server;
        return the_one_server;
    }

    AbstractServer* server_ = nullptr;
};

} // namespace ecf::service

#endif /* ecflow_service_Registry_HPP */
