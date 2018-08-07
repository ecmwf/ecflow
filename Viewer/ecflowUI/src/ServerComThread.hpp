//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERCOMTHREAD_HPP_
#define SERVERCOMTHREAD_HPP_

#include <deque>
#include <utility>
#include <sstream>
#include <string>
#include <vector>

#include "Defs.hpp"
#include "AbstractObserver.hpp"

#include "VTask.hpp"

#include <QThread>

class ChangeMgrSingleton;
class ClientInvoker;
class ServerComQueue;
class ServerHandler;

// -------------------------------------------------------
// ServerComThread - a class to handler communication with
// an ecflow server.
// -------------------------------------------------------

class ServerComThread : public QThread, public AbstractObserver
{
	Q_OBJECT

public:
	ServerComThread(ServerHandler *server, ClientInvoker *ci);
	~ServerComThread();

	void task(VTask_ptr);

	//From AbstractObserver
    virtual void update_start(const Node*, const std::vector<ecf::Aspect::Type>&) {}
    virtual void update_start(const Defs*, const std::vector<ecf::Aspect::Type>&) {}
	void update(const Node*, const std::vector<ecf::Aspect::Type>&);
	void update(const Defs*, const std::vector<ecf::Aspect::Type>&);
	void update_delete(const Node*);
	void update_delete(const Defs*);

Q_SIGNALS:
	void nodeChanged(const Node*, std::vector<ecf::Aspect::Type>);
	void defsChanged(std::vector<ecf::Aspect::Type>);
	void rescanNeed();
	void failed(std::string message);
	void suiteListChanged(const std::vector<std::string>&,const std::vector<std::string>&);

protected:
	void run();
	void reset();
	void sync_local();

private:
    void attach();
    void attach(defs_ptr d);
    void attach(Node *node);
    void detach();
    void detach(defs_ptr d);
    void detach(Node *node);
    void aspectToStr(std::stringstream& s,const std::vector<ecf::Aspect::Type>& t) const;

	ServerHandler *server_;
    std::string serverName_;
	ClientInvoker *ci_;
	VTask::Type taskType_;
	std::vector<std::string> command_;
	std::map<std::string,std::string> params_;
	std::vector<std::string> contents_;
	NameValueVec vars_;
    Zombie zombie_;
	std::string nodePath_;
	bool rescanNeed_;	
    bool hasSuiteFilter_;
	std::vector<std::string> filteredSuites_;
	bool autoAddNewSuites_;
	int maxLineNum_;
    bool initialResetDone_;
};

#endif
