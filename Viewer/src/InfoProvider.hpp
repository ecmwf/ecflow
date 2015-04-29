//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPROVIDER_HPP_
#define INFOPROVIDER_HPP_

#include "VInfo.hpp"
#include "InfoPresenter.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class InfoPanelItem;

class InfoProvider : public VTaskObserver, public VInfoVisitor
{
public:
	InfoProvider(InfoPresenter* owner,VTask::Type,const std::string& fileVarName="");
	virtual ~InfoProvider();

	void info(VInfo_ptr);

	//From VInfoVisitor
	void visit(VInfoServer*);
	void visit(VInfoNode*);
	void visit(VInfoAttribute*) {};

	//From VTaskObserver
	void taskChanged(VTask_ptr);

protected:
	InfoPresenter* owner_;
	VInfo_ptr info_;
	VTask_ptr task_;
	VReply* reply_;
	VTask::Type taskType_;
	std::string fileVarName_;
	//std::map<std::string,std::string> params_;
};

class JobProvider : public InfoProvider
{
public:
	 JobProvider(InfoPresenter* owner) :
		 InfoProvider(owner,VTask::JobTask,"ECF_JOB") {}
};


class ManualProvider : public InfoProvider
{
public:
	 ManualProvider(InfoPresenter* owner) :
		 InfoProvider(owner,VTask::ManualTask,"ECF_MANUAL") {}
};

class MessageProvider : public InfoProvider
{
public:
	 MessageProvider(InfoPresenter* owner) :
		 InfoProvider(owner,VTask::MessageTask) {}
};

class ScriptProvider : public InfoProvider
{
public:
	 ScriptProvider(InfoPresenter* owner) :
		 InfoProvider(owner,VTask::ScriptTask,"ECF_SCRIPT") {}
};


#endif
