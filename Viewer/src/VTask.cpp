//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VTask.hpp"
#include "VNode.hpp"
#include "VReply.hpp"
#include "VTaskObserver.hpp"
#include "Zombie.hpp"

VTask::VTask(Type t,VTaskObserver* obs) :
	type_(t),
	status_(NOSTATUS),
	targetPath_("/"),
	node_(0),
	reply_(0)
{
	if(obs)
		observers_.push_back(obs);
	reply_=new VReply();
}

VTask::VTask(Type t,VNode *node,VTaskObserver* obs) :
	type_(t),
	status_(NOSTATUS),
	node_(node),
	reply_(0)
{
	if(node_)
		targetPath_=node_->absNodePath();

	if(obs)
		observers_.push_back(obs);

	reply_=new VReply();
}

VTask::~VTask()
{
	delete reply_;
}

//Task for a server
VTask_ptr VTask::create(Type t,VTaskObserver* obs)
{
	return VTask_ptr(new VTask(t,obs));
}

//Task for a node
VTask_ptr VTask::create(Type t,VNode *node,VTaskObserver* obs)
{
	return VTask_ptr(new VTask(t,node,obs));
}


const std::string& VTask::typeString() const
{
	static std::map<Type,std::string> names;
	if(names.empty())
	{
		names[NoTask]="notask";
		names[CommandTask]="command";
		names[OverviewTask]="overview";
		names[WhyTask]="why";
		names[ManualTask]="manual";
		names[ScriptTask]="script";
		names[JobTask]="job";
		names[MessageTask]="message";
		names[OutputTask]="output";
		names[StatsTask]="stats";
		names[NewsTask]="news";
		names[SyncTask]="sync";
	}

	if(names.find(type_) != names.end())
		return names[type_];

	return names[NoTask];
}

const std::string& VTask::param(const std::string& key) const
{
	static std::string emptyStr("");
	std::map<std::string,std::string>::const_iterator it=params_.find(key);
	if(it != params_.end())
		return it->second;
	return emptyStr;
}

void VTask::status(Status s, bool broadcastIt)
{
	status_=s;
	if(broadcastIt)
		broadcast();
}

void VTask::setZombie(const Zombie& z)
{
    zombie_=z;
}

void VTask::removeObserver(VTaskObserver* o)
{
    std::vector<VTaskObserver*>::iterator it=std::find(observers_.begin(), observers_.end(),o);
    if(it != observers_.end())
    {
        observers_.erase(it);
    }
}

void VTask::broadcast()
{
	//VTask always exists as a shared pointer, so it is safe to pass on
	//this shared pointer to the observer.    
    for(std::vector<VTaskObserver*>::iterator it=observers_.begin(); it != observers_.end(); ++it)
	{
        (*it)->taskChanged(shared_from_this());
	}
}
