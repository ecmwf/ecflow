//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "VFile.hpp"

VFile::VFile(const std::string& name,const std::string& str,bool deleteFile) :
	path_(name),
	deleteFile_(deleteFile)
{
	std::ofstream f(path_.c_str());
	if(f.is_open())
	{
		f << str;
		f.close();
	}
}

VFile::VFile(const std::string& name,bool deleteFile) :
	path_(name),
	deleteFile_(deleteFile)
{
}

VFile::VFile(bool deleteFile) :
	path_(VFile::tmpName()),
	deleteFile_(deleteFile)
{
}

VFile::~VFile()
{
	if(deleteFile_)
	{
		//TODO: add further checks
		if(!path_.empty() && path_ != "/" && path_.size() > 4)
			unlink(path_.c_str());
	}
}

bool VFile::exists() const
{
	return (access(path_.c_str(), R_OK) ==0);
}

VFile_ptr VFile::create(const std::string& path,const std::string& str,bool deleteFile)
{
	return VFile_ptr(new VFile(path,str,deleteFile));
}

VFile_ptr VFile::create(const std::string& path,bool deleteFile)
{
	return VFile_ptr(new VFile(path,deleteFile));
}

VFile_ptr VFile::create(bool deleteFile)
{
	return VFile_ptr(new VFile(deleteFile));
}



std::string VFile::tmpName()
{
	std::string res;

#if defined(linux) || defined(_AIX)

	char *path = getenv("SCRATCH");
	char *s = (char*) malloc(128);

	if (!path || !access(path, R_OK))
	    path=getenv("TMPDIR");

	if (!path || !access(path, R_OK))
	    path=(char*)"/tmp";

	snprintf(s, 128, "%s/%sXXXXXX", path, "ecflow_ui");
	if(mkstemp(s) != -1)
	{
		res=std::string(s);
	}

	free(s);

#else

	char* s=std::string(tmpnam(NULL));
	res=std::string(s);

#endif

	return res;

}

