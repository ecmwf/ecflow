//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VFILE_H
#define VFILE_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class VFile;
typedef boost::shared_ptr<VFile> VFile_ptr;

class VFile : public boost::enable_shared_from_this<VFile>
{
public:
	virtual ~VFile();

	const std::string& path() const {return path_;}
	void  setContents(const std::string);
	bool exists() const;

	static VFile_ptr create(const std::string& path,const std::string& contents,bool deleteFile=true);
	static VFile_ptr create(const std::string& path,bool deleteFile= true);
	static VFile_ptr create(bool deleteFile= true);

	static std::string tmpName();

protected:
	VFile(const std::string& name,const std::string& str,bool deleteFile=true);
	VFile(const std::string& str,bool deleteFile= true);
	VFile(bool deleteFile= true);

	std::string path_;
	bool  deleteFile_;
};

#endif
