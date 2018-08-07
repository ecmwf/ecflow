//============================================================================
// Copyright 2009-2017 ECMWF.
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
	for(const auto & it : msg)
	{
		text_+=it + "\n";
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

std::string VReply::errorText(const std::string& sep) const
{
    std::string s;
    for(size_t i=0; i < errorText_.size(); i++)
    {
        if(i != 0) s+=sep;
        s+=errorText_[i];
    }
    return s;
}

std::string VReply::warningText(const std::string& sep) const
{
    std::string s;
    for(size_t i=0; i < warningText_.size(); i++)
    {
        if(i != 0) s+=sep;
        s+=warningText_[i];
    }
    return s;
}

std::string VReply::infoText(const std::string& sep) const
{
    std::string s;
    for(size_t i=0; i < infoText_.size(); i++)
    {
        if(i != 0) s+=sep;
        s+=infoText_[i];
    }
    return s;
}

void VReply::setErrorText(const std::string& s)
{
    errorText_.clear();
    errorText_.push_back(s);
}

void VReply::appendErrorText(const std::string& s)
{
    errorText_.push_back(s);
}

void VReply::setWarningText(const std::string& s)
{
    warningText_.clear();
    warningText_.push_back(s);
}

void  VReply::appendWarningText(const std::string& s)
{
    warningText_.push_back(s);
}

void VReply::setInfoText(const std::string& s)
{
    infoText_.clear();
    infoText_.push_back(s);
}

void  VReply::appendInfoText(const std::string& s)
{
    infoText_.push_back(s);
}

void VReply::reset()
{
	status_=NoStatus;
	text_.clear();
	errorText_.clear();
	warningText_.clear();
	infoText_.clear();
	fileName_.clear();
	zombies_.clear();
	readMode_=NoReadMode;
	readMethod_.clear();
	tmpFile_.reset();
    log_.clear();
}
