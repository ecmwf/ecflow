//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVEROBSERVER_HPP_
#define SERVEROBSERVER_HPP_

#include <vector>
#include "Aspect.hpp"

class ServerHandler;
class VServerChange;

class ServerObserver
{
public:
    ServerObserver() {}
    virtual ~ServerObserver() {}
	virtual void notifyDefsChanged(ServerHandler* server, const std::vector<ecf::Aspect::Type>& a)=0;
	virtual void notifyServerDelete(ServerHandler* server)=0;
    virtual void notifyBeginServerClear(ServerHandler* server) {}
    virtual void notifyEndServerClear(ServerHandler* server) {}
    virtual void notifyBeginServerScan(ServerHandler* server,const VServerChange&) {}
    virtual void notifyEndServerScan(ServerHandler* server) {}
    virtual void notifyServerConnectState(ServerHandler* server) {}
    virtual void notifyServerActivityChanged(ServerHandler* server) {}
    virtual void notifyServerSuiteFilterChanged(ServerHandler* server) {}
    virtual void notifyEndServerSync(ServerHandler* server) {}
};


#endif
