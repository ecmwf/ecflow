//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERCOMQUEUE_HPP_
#define SERVERCOMQUEUE_HPP_

#include <deque>
#include <vector>

#include "VTask.hpp"

#include <QObject>
#include <QTime>
#include <QTimer>

class ClientInvoker;
class NodeObserver;
class ServerHandler;
class ServerComThread;

// --------------------------------------------------------------
// ServerComQueue - a class to provide a queueing system for
// sending tasks to the ClientIvoker via the ServerComThread.
// --------------------------------------------------------------

class ServerComQueue : public QObject
{
Q_OBJECT

public:
    ServerComQueue(ServerHandler *server,ClientInvoker* client);
	~ServerComQueue() override;

	enum State {NoState,RunningState,SuspendedState,ResetState,DisabledState};
	State state() const {return state_;}

	void addTask(VTask_ptr);
	void addNewsTask();
	void addSyncTask();
	void addSuiteListTask();
	void addSuiteAutoRegisterTask();

	void enable();
	void disable();
	void start();
	void suspend(bool);
	bool prepareReset();
	void reset();
	bool isSuspended() const {return state_==SuspendedState;}

protected Q_SLOTS:
	void slotRun();
    void slotTaskStarted();
	void slotTaskFinished();
	void slotTaskFailed(std::string);

protected:
    void createThread();
    void stopTimer();
    void startCurrentTask();
    void endReset();
	bool hasTask(VTask::Type t) const;
	bool isNextTask(VTask::Type t) const;

	ServerHandler *server_;
	ClientInvoker* client_;
	ServerComThread *comThread_;
	QTimer* timer_;
    int timeout_;
    QTime ctStartTime_;
    int ctStartTimeout_;
    int ctStartWaitTimeout_;
    int startTimeoutTryCnt_;
    int ctMaxStartTimeoutTryCnt_;
	std::deque<VTask_ptr> tasks_;
	VTask_ptr current_;
	State state_;  
    bool taskStarted_;
    bool taskIsBeingFinished_;
	bool taskIsBeingFailed_;
};

#endif
