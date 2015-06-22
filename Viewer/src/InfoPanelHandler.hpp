//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef INFOPANELHANDLER_HPP_
#define INFOPANELHANDLER_HPP_

#include <string>
#include <vector>

#include "VInfo.hpp"

class BaseNodeCondition;

class InfoPanelDef
{
public:
	InfoPanelDef(const std::string&);

	std::string name() const {return name_;}
	std::string label() const {return label_;}
	BaseNodeCondition* visibleCondition() const {return visibleCondition_;}

	void setLabel(const std::string& s) {label_=s;}
	void setIcon(const std::string& s) {icon_=s;}
	void setVisibleCondition(BaseNodeCondition *visibleCond) {visibleCondition_=visibleCond;}
	void setEnabledCondition(BaseNodeCondition *enabledCond) {enabledCondition_=enabledCond;}

protected:
	std::string name_;
	std::string label_;
	std::string icon_;

	BaseNodeCondition *visibleCondition_;
	BaseNodeCondition *enabledCondition_;
};

class InfoPanelHandler
{
public:
	InfoPanelHandler();

	void init(const std::string& file);
	void visible(VInfo_ptr info,std::vector<InfoPanelDef*>& lst);

	static InfoPanelHandler* instance();

protected:
	static InfoPanelHandler* instance_;

	std::vector<InfoPanelDef*> panels_;

};

#endif
