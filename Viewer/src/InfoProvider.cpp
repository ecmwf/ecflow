//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "InfoProvider.hpp"
#include "VReply.hpp"

InfoProvider::InfoProvider(InfoPresenter* owner) : owner_(owner)
{
	reply_=new VReply();
}

InfoProvider::~InfoProvider()
{
	delete reply_;
	if(task_)
		task_->status(VTask::CANCELLED);
}

void InfoProvider::info(VInfo_ptr info)
{
	//We keep it alive
	info_=info;

	if(task_)
	{
		task_->status(VTask::CANCELLED);
		task_.reset();
	}

	if(owner_ && info_)
		info_->accept(this);
}
