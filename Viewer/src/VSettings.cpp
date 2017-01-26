//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VSettings.hpp"

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"
#include "UserMessage.hpp"

#include <QDebug>

#include <sstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

//#define _UI_SETTINGS_DEBUG

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

std::string VSettingsPath::join(const std::string& sep) const
{
	return boost::algorithm::join(path_,sep);
}

//======================================================
//
// VSettings
//
//======================================================

VSettings::VSettings(const std::string& file) : file_(file)
{
}

VSettings::VSettings(boost::property_tree::ptree pt) : pt_(pt)
{
}


void VSettings::clear()
{
	pt_.clear();

}

bool VSettings::contains(const std::string& key)
{
	return (pt_.get_child_optional(path_.path(key)) != boost::none);
}

bool VSettings::containsFullPath(const std::string& key)
{
	return (pt_.get_child_optional(key) != boost::none);
}


//bool VSettings::read(const std::string &fs)
bool VSettings::read(bool failIfFileDoesNotExist)
{
	try
	{
		boost::property_tree::json_parser::read_json(file_,pt_);
	}
	catch (const boost::property_tree::json_parser::json_parser_error& e)
	{
		namespace fs=boost::filesystem;
		fs::path boostpath(file_);
		if (!failIfFileDoesNotExist && !fs::exists(boostpath))  // no file and that's ok
			return false;

		if(!DirectoryHandler::isFirstStartUp())
		{
			std::string errorMessage = e.what();
			UserMessage::message(UserMessage::ERROR, true, std::string("Error, unable to parse JSON session file : " + errorMessage));
		}
		return false;
	}

	return true;
}

//void VSettings::write(const std::string &fs)
void VSettings::write()
{
	//Write to json
	write_json(file_,pt_);
}

void VSettings::put(const std::string& key,int val)
{
	pt_.put(path_.path(key),val);
}

/*void VSettings::put(const std::string& key,bool val)
{
	pt_.put(path_.path(key),val);
}*/

void VSettings::put(const std::string& key,const std::string& val)
{
	pt_.put(path_.path(key),val);
}


void VSettings::put(const std::string& key,const std::vector<std::string>& val)
{
	boost::property_tree::ptree array;
	for(std::vector<std::string>::const_iterator it=val.begin(); it != val.end(); ++it)
	{
		array.push_back(std::make_pair("",(*it)));
	}
	pt_.put_child(path_.path(key),array);
}

void VSettings::put(const std::string& key,const std::vector<int>& val)
{
    boost::property_tree::ptree array;
    for(std::vector<int>::const_iterator it=val.begin(); it != val.end(); ++it)
    {
        std::stringstream ss;
        ss << (*it);
        array.push_back(std::make_pair("",ss.str()));
    }
    pt_.put_child(path_.path(key),array);
}

// for adding a list of 'structs'
void VSettings::put(const std::string& key,const std::vector<VSettings>& val)
{
	boost::property_tree::ptree array;
	for(std::vector<VSettings>::const_iterator it=val.begin(); it != val.end(); ++it)
	{
		array.push_back(std::make_pair("",(*it).pt_));
	}
	pt_.put_child(path_.path(key),array);
}


void VSettings::putAsBool(const std::string& key,bool val)
{
	pt_.put(path_.path(key),val?"true":"false");
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

void VSettings::get(const std::string& key,std::vector<int>& val)
{
    boost::optional<boost::property_tree::ptree& > ptArray=pt_.get_child_optional(path_.path(key));
    if(!ptArray)
    {
        return;
    }

    for(boost::property_tree::ptree::const_iterator it = ptArray.get().begin(); it != ptArray.get().end(); ++it)
    {
        val.push_back(it->second.get_value<int>());
    }
}

bool VSettings::getAsBool(const std::string& key,bool defaultVal)
{
	std::string v=pt_.get<std::string>(path_.path(key),(defaultVal)?"true":"false");
	if(v == "true" || v == "1")
		return true;

	return false;

}

// for getting a list of 'structs'
void VSettings::get(const std::string& key, std::vector<VSettings>& val)
{
	boost::optional<boost::property_tree::ptree& > ptArray=pt_.get_child_optional(path_.path(key));
	if(!ptArray)
	{
		return;
	}

	for(boost::property_tree::ptree::const_iterator it = ptArray.get().begin(); it != ptArray.get().end(); ++it)
	{
		boost::property_tree::ptree child = it->second;
		VSettings vs(child);
		val.push_back(vs);
	}
}


void VSettings::beginGroup(const std::string &id)
{
	path_.add(id);
}

void VSettings::endGroup()
{
	path_.pop();
}


//======================================================
//
// VSettings
//
//======================================================

VComboSettings::VComboSettings(const std::string& file,const std::string& qsFile) :
		VSettings(file),
		qs_(QString::fromStdString(qsFile),QSettings::NativeFormat)
{

	//qs_(QString::fromStdString(application))
	//QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,"/home/graphics/cgr/.ecflowview");
	//QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,"/home/graphics/cgr/.ecflowview");
#ifdef _UI_SETTINGS_DEBUG
    UiLog().dbg() << "VComboSettings --> fileName=" << qs_.fileName();
#endif
}

VComboSettings::~VComboSettings()
{
}

void VComboSettings::clear()
{
	VSettings::clear();
    qs_.clear();
}

bool VComboSettings::containsQs(const std::string& key)
{
	return qs_.contains(QString::fromStdString(key));
}

void VComboSettings::write()
{
	VSettings::write();
	qs_.sync();
}

void VComboSettings::putQs(const std::string& key,QVariant val)
{
	qs_.setValue(QString::fromStdString(key),val);
}

QVariant  VComboSettings::getQs(const std::string& key)
{
#ifdef _UI_SETTINGS_DEBUG
    qDebug() << "qt group" << qs_.group();
#endif
	return qs_.value(QString::fromStdString(key));
}

void VComboSettings::beginGroup(const std::string &id)
{
	VSettings::beginGroup(id);
	qs_.beginGroup(QString::fromStdString(id));
#ifdef _UI_SETTINGS_DEBUG
	qDebug() << "qt group" << qs_.group();
#endif
}

void VComboSettings::endGroup()
{
	VSettings::endGroup();
	qs_.endGroup();
}
