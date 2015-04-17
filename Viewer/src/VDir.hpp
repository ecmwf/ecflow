//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VDIR_H
#define VDIR_H

#include <string>
#include <vector>
#include <ctime>

#include <boost/shared_ptr.hpp>

class VDirItem
{
public:
	std::string name_;
  	//int mode_;
  	//int uid_;
    //int gid_;
  	unsigned int size_;
  	//int atime_;
  	std::time_t mtime_;
  	//int ctime_;
  	std::string method_;
};

class VDir
{
public:
	VDir(const std::string& path);
	VDir(const std::string& path, const std::string& pattern);

	~VDir();

	const std::string& path() const {return path_;}
    void path(const std::string path,bool reload=true);

    const std::string& where() const {return where_;}
    void where(const std::string w) {where_=w;}

    std::string fullName(int row);

	int count() const {return static_cast<int>(items_.size());}
	void clear();
	void reload();
	void addItem(const std::string&, unsigned int, unsigned int);
	const std::vector<VDirItem*>& items() const {return items_;}

protected:
	std::string path_;
	std::string pattern_;
	std::string where_;
	std::vector<VDirItem*> items_;
};

class VDir;
typedef boost::shared_ptr<VDir> VDir_ptr;

#endif
