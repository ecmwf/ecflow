//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <algorithm>

#include "NodeQueryHandler.hpp"

#include "DirectoryHandler.hpp"
#include "File.hpp"
#include "NodeQuery.hpp"

NodeQueryHandler* NodeQueryHandler::instance_=0;

NodeQueryHandler::NodeQueryHandler() : suffix_("query")
{
}

NodeQueryHandler* NodeQueryHandler::instance()
{
	if(!instance_)
	{
		instance_=new NodeQueryHandler();
	}

	return instance_;
}

NodeQuery* NodeQueryHandler::add(const std::string& name)
{
	NodeQuery *item=new NodeQuery(name);
	items_.push_back(item);
	return item;
}

NodeQuery* NodeQueryHandler::add(const std::string& name,const std::string& query)
{
	NodeQuery *item=new NodeQuery(name);
	items_.push_back(item);
	return item;
}

void NodeQueryHandler::add(NodeQuery* item,bool saveToFile)
{
	items_.push_back(item);
	if(saveToFile)
		save(item);
}


void NodeQueryHandler::remove(const std::string&)
{
}

void NodeQueryHandler::remove(NodeQuery*)
{

}

NodeQuery* NodeQueryHandler::find(const std::string& name) const
{
	for(std::vector<NodeQuery*>::const_iterator it=items_.begin(); it != items_.end(); ++it)
	{
		if((*it)->name() == name)
			return *it;
	}
	return NULL;
}

void NodeQueryHandler::save()
{
}

void NodeQueryHandler::save(NodeQuery *item)
{
	std::string f=DirectoryHandler::concatenate(dirPath_,item->name() + "." + suffix_);
	VSettings vs(f);
	item->save(&vs);
	vs.write();

}

void NodeQueryHandler::init(const std::string& dirPath)
{
	dirPath_=dirPath;
	DirectoryHandler::createDir(dirPath_);

	std::vector<std::string> res;
	std::string pattern=".*\\." + suffix_ + "$";
	DirectoryHandler::findFiles(dirPath_,pattern,res);

	for(std::vector<std::string>::const_iterator it=res.begin(); it != res.end(); ++it)
	{
		std::string fName=DirectoryHandler::concatenate(dirPath_,*it);
		VSettings vs(fName);
		vs.read();

		std::size_t pos=(*it).find("." + suffix_);
		assert(pos != std::string::npos);

		std::string name=(*it).substr(0,pos);
		NodeQuery* item=add(name);
		item->load(&vs);
	}
}




