//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "QueryHandler.hpp"

#include <algorithm>

#include "DirectoryHandler.hpp"
#include "File.hpp"

QueryHandler* QueryHandler::instance_=0;

QueryItem::QueryItem(const std::string& name) :
  name_(name)
{

}

QueryItem::QueryItem(const std::string& name,const std::string& query) :
  name_(name),
  query_(query)
{

}

void  QueryItem::setName(const std::string& name)
{
	name_=name;
}

void  QueryItem::setQuery(const std::string& query)
{
	query_=query;
}

//=================================================
//
// QueryHandler
//
//=================================================

QueryHandler::QueryHandler()
{
}

QueryHandler* QueryHandler::instance()
{
	if(!instance_)
	{
		instance_=new QueryHandler();
	}

	return instance_;
}

QueryItem* QueryHandler::add(const std::string& name)
{
	QueryItem *item=new QueryItem(name);
	items_.push_back(item);
	return item;
}

QueryItem* QueryHandler::add(const std::string& name,const std::string& query)
{
	QueryItem *item=new QueryItem(name,query);
	items_.push_back(item);
	return item;
}


void QueryHandler::remove(const std::string&)
{
}

void QueryHandler::remove(QueryItem*)
{

}

void QueryHandler::save()
{
}

void QueryHandler::init(const std::string& dirPath)
{
	dirPath_=dirPath;
	DirectoryHandler::createDir(dirPath_);

	std::vector<std::string> res;
	std::string pattern=".*\\.query$";
	DirectoryHandler::findFiles(dirPath_,pattern,res);

	for(std::vector<std::string>::const_iterator it=res.begin(); it != res.end(); ++it)
	{
		std::string f=DirectoryHandler::concatenate(dirPath_,*it);
		std::string query;
		if(ecf::File::open(f,query))
		{
			add(*it,query);
		}
	}
}
