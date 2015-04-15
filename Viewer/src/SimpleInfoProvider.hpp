//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SIMPLEINFOPROVIDER_HPP_
#define SIMPLEINFOPROVIDER_HPP_

#include "InfoProvider.hpp"

class InfoPanelItem;

class SimpleInfoProvider : public InfoProvider
{
public:
    SimpleInfoProvider(InfoPresenter* owner,VTask::Type);

    //From VInfoVisitor
    void visit(VInfoServer*) {};
    void visit(VInfoNode*);
    void visit(VInfoAttribute*) {};

    //From VTaskObserver
    void taskChanged(VTask_ptr);

private:
    VTask::Type taskType_;
};

class ManualProvider : public SimpleInfoProvider
{
public:
	 ManualProvider(InfoPresenter* owner,VTask::Type taskType) :
		 SimpleInfoProvider(owner,VTask::ManualTask) {}
};



#endif
