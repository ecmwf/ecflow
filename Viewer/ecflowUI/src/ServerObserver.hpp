/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_ServerObserver_HPP
#define ecflow_viewer_ServerObserver_HPP

#include <string>
#include <vector>

#include "Aspect.hpp"

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
