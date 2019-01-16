//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERCOMOBSERVER_HPP
#define SERVERCOMOBSERVER_HPP

class ServerHandler;

class ServerComObserver
{
public:
    ServerComObserver() = default;
    virtual ~ServerComObserver() = default;

    virtual void notifyRefreshTimerStarted(ServerHandler* server) {}
    virtual void notifyRefreshTimerStopped(ServerHandler* server) {}
    virtual void notifyRefreshTimerChanged(ServerHandler* server) {}
    virtual void notifyRefreshScheduled(ServerHandler* server) {}
    virtual void notifyRefreshFinished(ServerHandler* server) {}
};

#endif // SERVERCOMOBSERVER_HPP

