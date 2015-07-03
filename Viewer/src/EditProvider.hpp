//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef EDITPROVIDER_HPP_
#define EDITPROVIDER_HPP_

#include "VDir.hpp"
#include "VInfo.hpp"
#include "InfoProvider.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class EditProvider : public InfoProvider
{
public:
	 explicit EditProvider(InfoPresenter* owner) :
		 InfoProvider(owner,VTask::OutputTask), preproc_(false) {}

	 void visit(VInfoNode*);
	 void submit(const std::vector<std::string>& txt,bool alias);

	 void preproc(bool b) {preproc_=b;}

private:
	 bool preproc_;
};

#endif
