//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================


#include "StringMatchMode.hpp"

std::map<StringMatchMode::Mode,std::string> StringMatchMode::matchOper_;

StringMatchMode::StringMatchMode() :
  mode_(WildcardMatch)
{
	init();
}

StringMatchMode::StringMatchMode(Mode mode) :
  mode_(mode)
{
	init();
}

void StringMatchMode::init()
{
	if(matchOper_.empty())
	{
		matchOper_[ContainsMatch]="~";
		matchOper_[WildcardMatch]="=";
		matchOper_[RegexpMatch]="=~";
	}
}

const std::string& StringMatchMode::matchOperator() const
{
	static std::string emptyStr;
	std::map<Mode,std::string>::const_iterator it=matchOper_.find(mode_);
	if(it != matchOper_.end())
		return it->second;

	return emptyStr;
}

StringMatchMode::Mode StringMatchMode::operToMode(const std::string& op)
{
	for(std::map<Mode,std::string>::const_iterator it=matchOper_.begin(); it != matchOper_.end(); ++it)
	{
		if(op == it->second)
			return it->first;
	}
	return WildcardMatch;
}
