//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VTASK_HPP_
#define VTASK_HPP_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

//#include "VReply.hpp"
class VReply;

class Node;
class VNode;
class VTaskObserver;

//This class defines a task that can be sent to the server to be executed.
//A VTask can only be created through the static create methods that return back
//a shared pointer of VTask. So VTask can only exist as a shared pointer.
//VTask contains a

class VTask;
typedef boost::shared_ptr<VTask> VTask_ptr;

class VTask  : public boost::enable_shared_from_this<VTask>
{
public:
	enum Type {NoTask,CommandTask,OverviewTask,WhyTask,ManualTask,ScriptTask,
		       JobTask,MessageTask,OutputTask,StatsTask,NewsTask,SyncTask};
	enum Status {NOSTATUS,QUEUED,RUNNING,FINISHED,CANCELLED,ABORTED,REJECTED};

	virtual ~VTask();

	Type type() const {return type_;}
	const std::string& typeString() const;
	VNode* node() const {return node_;}
	Status status() {return status_;}
	const std::string& targetPath() const {return targetPath_;}

	const std::string& param(const std::string& key) const;
	const std::map<std::string,std::string>& params() const {return params_;}
	const std::vector<std::string>& command() const {return command_;}
	VReply* reply() const {return reply_;}

	void param(const std::string& key,const std::string& val) {params_[key]=val;}
	void command(const std::vector<std::string>& cmd) {command_=cmd;}

	//When it is called the observers are notified about the change in status.
	void status(Status s,bool broadcast=true);

	void aborted(const std::string&,bool broadcast=true) {};
	void broadcast();

	static VTask_ptr create(Type t,VTaskObserver* obs=0);
	static VTask_ptr create(Type t,VNode *node,VTaskObserver* obs=0);

protected:
	VTask(Type t,VTaskObserver* obs=0);
	VTask(Type t,VNode *node,VTaskObserver* obs=0);

	Type type_;
	Status status_;
	std::map<std::string,std::string> params_;
	std::vector<std::string> command_;
	std::string targetPath_;
	VNode *node_;
	std::vector<VTaskObserver*> observers_;
	VReply*  reply_;
};

#endif
