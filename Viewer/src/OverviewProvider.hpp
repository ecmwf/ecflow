//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef OVERVIEWPROVIDER_HPP_
#define OVERVIEWPROVIDER_HPP_

#include "InfoProvider.hpp"

class InfoPanelItem;

class OverviewProvider : public InfoProvider
{
public:
	OverviewProvider(InfoPresenter* owner);

	//From VInfoVisitor
	void visit(VInfoServer*);
	void visit(VInfoNode*);
	void visit(VInfoAttribute*);

	//From VTaskObserver
	void taskChanged(VTask_ptr);

protected:
	void serverInfo(VInfoServer*,std::stringstream& f);
	void nodeInfo(VInfoNode*,std::stringstream& f);

};

#endif
