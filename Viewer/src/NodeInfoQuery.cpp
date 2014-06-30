//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "NodeInfoQuery.hpp"

#include "File.hpp"

bool NodeInfoQuery::readFile()
{
	if(!fileName_.empty() &&
	   ecf::File::open(fileName_,text_))
	{
		return true;
	}

	return false;
}

void NodeInfoQuery::text(const std::vector<std::string>& msg)
{
	text_.clear();
	for(std::vector<std::string>::const_iterator it=msg.begin(); it != msg.end(); it++)
	{
			text_+=*it + "\n";
	}
}
