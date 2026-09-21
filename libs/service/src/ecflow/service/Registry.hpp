// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
