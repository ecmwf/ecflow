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
	InfoProvider(InfoPresenter* owner,VTask::Type);
	virtual ~InfoProvider();

	void info(VInfo_ptr);
	void command(VTask::Type);
	virtual void clear();

	bool enabled() const {return enabled_;}
	void setEnabled(bool);
	bool autoUpdate() const {return autoUpdate_;}
	void setAutoUpdate(bool);
	bool inAutoUpdate() const {return inAutoUpdate_;}

	//From VInfoVisitor
	void visit(VInfoServer*);
	void visit(VInfoNode*);
	void visit(VInfoAttribute*) {};

	//From VTaskObserver
	void taskChanged(VTask_ptr);

protected:
	virtual void handleFileNotDefined(VReply *reply);
	virtual bool handleFileMissing(const std::string& fileName,VReply *reply);
	virtual void optionsChanged() {}

	InfoPresenter* owner_;
	VInfo_ptr info_;
	VTask_ptr task_;
	VReply* reply_;
	VTask::Type taskType_;
	std::string fileVarName_;
	std::string fileNotDefinedText_;
	std::string fileMissingText_;
	bool enabled_;
	bool autoUpdate_;
	bool inAutoUpdate_;
};

class JobProvider : public InfoProvider
{
public:
	 explicit JobProvider(InfoPresenter* owner);
protected:
	 bool handleFileMissing(const std::string& fileName,VReply *reply);
};

class ManualProvider : public InfoProvider
{
public:
	 explicit ManualProvider(InfoPresenter* owner);
};

class MessageProvider : public InfoProvider
{
public:
	 explicit MessageProvider(InfoPresenter* owner);
};

class ScriptProvider : public InfoProvider
{
public:
	 explicit ScriptProvider(InfoPresenter* owner);
};

class HistoryProvider : public InfoProvider
{
public:
	 explicit HistoryProvider(InfoPresenter* owner);
};

class ZombieProvider : public InfoProvider
{
public:
	 explicit ZombieProvider(InfoPresenter* owner);
};


#endif
