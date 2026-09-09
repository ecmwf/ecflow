/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerObserver_HPP
#define ecflow_viewer_ServerObserver_HPP

#include <string>
#include <vector>

#include "ecflow/node/Aspect.hpp"

class ServerHandler;
class VServerChange;

class ServerObserver {
public:
    ServerObserver()                                                                               = default;
    virtual ~ServerObserver()                                                                      = default;
    virtual void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a) = 0;
    virtual void notifyServerDelete(ServerHandler* server)                                         = 0;
    virtual void notifyBeginServerClear(ServerHandler*) {}
    virtual void notifyEndServerClear(ServerHandler*) {}
    virtual void notifyBeginServerScan(ServerHandler*, const VServerChange&) {}
    virtual void notifyEndServerScan(ServerHandler*) {}
    virtual void notifyServerConnectState(ServerHandler*) {}
    virtual void notifyServerActivityChanged(ServerHandler*) {}
    virtual void notifyServerSuiteFilterChanged(ServerHandler*) {}
    virtual void notifyEndServerSync(ServerHandler*) {}
    virtual void notifyServerRenamed(ServerHandler*, const std::string& /*oldName*/) {}
};

#endif /* ecflow_viewer_ServerObserver_HPP */
