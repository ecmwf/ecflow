//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPRESENTER_HPP_
#define INFOPRESENTER_HPP_

#include "VInfo.hpp"

class InfoProvider;
class VReply;

class InfoPresenter
{
public:
	InfoPresenter() : infoProvider_(0) {};
	virtual ~InfoPresenter() {};
	virtual void infoReady(VReply*) {};
	virtual void infoFailed(VReply*) {};
	virtual void infoProgress(VReply*) {};
	VInfo_ptr info() const {return info_;}

protected:
	VInfo_ptr info_;
	InfoProvider* infoProvider_;
};

#endif
