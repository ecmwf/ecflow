//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VDir.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/foreach.hpp>

#include <boost/algorithm/string/predicate.hpp>


VDir::VDir(const std::string& path) : path_(path)
{
}

VDir::VDir(const std::string& path,const std::string& pattern) :
     path_(path),
     pattern_(pattern)
{
	reload();
}

VDir::~VDir()
{
	clear();
}

void VDir::clear()
{
	for(std::vector<VDirItem*>::iterator it=items_.begin(); it != items_.end(); it++)
			delete (*it);
}

void VDir::addItem(const char* name, unsigned int size,unsigned int mtime)
{
	VDirItem* item=new VDirItem;

	item->name_=std::string(name);
	item->size_=size;
	item->mtime_ = mtime;

	items_.push_back(item);

}

void VDir::reload()
{
	clear();

	boost::filesystem::path path(path_);

    boost::filesystem::directory_iterator it(path), eod;

    BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(it, eod ))
    {
        if(is_regular_file(p) && boost::algorithm::starts_with(p.filename().string(),pattern_))
        {
        	VDirItem* item=new VDirItem;

        	item->name_ = p.filename().string();
        	item->size_ =  boost::filesystem::file_size(p);
        	item->size_ =  boost::filesystem::file_size(p);
        	item->mtime_ = boost::filesystem::last_write_time(p);

        	items_.push_back(item);

        }
    }
}
