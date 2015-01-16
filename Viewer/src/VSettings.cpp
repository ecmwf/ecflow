//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VSettings.hpp"
#include "UserMessage.hpp"

#include <QDebug>

#include <boost/algorithm/string/join.hpp>
#include <boost/property_tree/json_parser.hpp>

//======================================================
//
// VSettingsPath
//
//======================================================

void VSettingsPath::add(const std::string& key)
{
	path_.push_back(key);
}

void VSettingsPath::pop()
{
	if(!path_.empty())
		path_.pop_back();
}

std::string VSettingsPath::path() const
{
	return join(".");
}

std::string VSettingsPath::path(const std::string& key) const
{
	std::string s=path();
	return (s.empty())?key:(s+"."+key);
}

std::string VSettingsPath::join(const std::string sep) const
{
	return boost::algorithm::join(path_,sep);
}

//======================================================
//
// VSettings
//
//======================================================

VSettings::VSettings(const std::string& application) :
		qs_("ECMWF",QString::fromStdString(application))
{
}

void VSettings::clear()
{
	qs_.clear();
	pt_.clear();

}

bool VSettings::contains(const std::string& key)
{
	return (pt_.get_child_optional(path_.path(key)) != boost::none);
}

bool VSettings::containsQs(const std::string& key)
{
	return qs_.contains(QString::fromStdString(key));
}

bool VSettings::read(const std::string &fs)
{
	try
	{
		boost::property_tree::json_parser::read_json(fs,pt_);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		 std::string errorMessage = e.what();
		 UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON session file : " + errorMessage));
		 return false;
	}

	return true;
}


void VSettings::write(const std::string &fs)
{
	//Write to json
	write_json(fs,pt_);

	qs_.sync();
}

void VSettings::put(const std::string& key,int val)
{
	pt_.put(path_.path(key),val);
}

void VSettings::put(const std::string& key,const std::vector<std::string>& val)
{
	boost::property_tree::ptree array;
	for(std::vector<std::string>::const_iterator it=val.begin(); it != val.end(); it++)
	{
		array.push_back(std::make_pair("",(*it)));
	}
	pt_.put_child(path_.path(key),array);
}

void VSettings::putQs(const std::string& key,QVariant val)
{
	qs_.setValue(QString::fromStdString(key),val);
}

void VSettings::get(const std::string& key,std::vector<std::string>& val)
{
	boost::optional<boost::property_tree::ptree& > ptArray=pt_.get_child_optional(path_.path(key));
	if(!ptArray)
	{
		return;
	}


	//boost::property_tree::ptree ptArray=it->second;
	for(boost::property_tree::ptree::const_iterator it = ptArray.get().begin(); it != ptArray.get().end(); ++it)
	{
			std::string name=it->second.get_value<std::string>();
			val.push_back(name);

	}
}

QVariant  VSettings::getQs(const std::string& key)
{
	return qs_.value(QString::fromStdString(key));
}

void VSettings::beginGroup(const std::string &id)
{
	path_.add(id);
	qs_.beginGroup(QString::fromStdString(id));
}

void VSettings::endGroup()
{
	path_.pop();
	qs_.endGroup();
}
