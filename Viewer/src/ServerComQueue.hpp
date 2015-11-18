//============================================================================
// Copyright 2015 ECMWF.
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
	ServerComQueue(ServerHandler *server,ClientInvoker* client,ServerComThread* comThread);
	~ServerComQueue();

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

protected Q_SLOTS:
	void slotTaskFinished();
	void slotTaskFailed(std::string);

protected:
	void endReset();
	bool hasTask(VTask::Type t) const;
	bool isNextTask(VTask::Type t) const;

	ServerHandler *server_;
	ClientInvoker* client_;
	ServerComThread *comThread_;
	QTimer* timer_;
	int timeout_;
	std::deque<VTask_ptr> tasks_;
	VTask_ptr current_;
	State state_;
	bool taskIsBeingFinished_;
	bool taskIsBeingFailed_;
};

#endif
