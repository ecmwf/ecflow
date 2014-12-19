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
	InfoProvider(InfoPresenter* owner);
	virtual ~InfoProvider();

	void info(VInfo_ptr);

	//From VTaskObserver
	void taskChanged(VTask_ptr) {};

protected:
	InfoPresenter* owner_;
	VInfo_ptr info_;
	VTask_ptr task_;
	VReply* reply_;
};


#endif
