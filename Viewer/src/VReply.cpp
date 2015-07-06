//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VReply.hpp"

#include "File.hpp"

bool VReply::textFromFile(const std::string& fileName)
{
	if(!fileName.empty() &&
	   ecf::File::open(fileName,text_))
	{
		return true;
	}

	return false;
}

void VReply::text(const std::vector<std::string>& msg)
{
	text_.clear();
	for(std::vector<std::string>::const_iterator it=msg.begin(); it != msg.end(); ++it)
	{
			text_+=*it + "\n";
	}
}

void VReply::prependText(const std::string& txt)
{
	text_.insert(0,txt);
}

void VReply::appendText(const std::string& txt)
{
	text_.append(txt);
}

void VReply::reset()
{
	text_.clear();
	errorText_.clear();
}
