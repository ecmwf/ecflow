/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ServerComObserver_HPP
#define ecflow_viewer_ServerComObserver_HPP

class ServerHandler;

class ServerComObserver {
public:
    ServerComObserver()          = default;
    virtual ~ServerComObserver() = default;

    virtual void notifyRefreshTimerStarted(ServerHandler*) {}
    virtual void notifyRefreshTimerStopped(ServerHandler*) {}
    virtual void notifyRefreshTimerChanged(ServerHandler*) {}
    virtual void notifyRefreshScheduled(ServerHandler*) {}
    virtual void notifyRefreshFinished(ServerHandler*) {}
};

#endif /* ecflow_viewer_ServerComObserver_HPP */
