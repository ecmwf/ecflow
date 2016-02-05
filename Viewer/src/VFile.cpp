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
#include <string.h>

#include "VFile.hpp"
const size_t VFile::maxDataSize_=1024*1024*10;

VFile::VFile(const std::string& name,const std::string& str,bool deleteFile) :
	path_(name),
	deleteFile_(deleteFile),
	storageMode_(DiskStorage),
	data_(0),
	dataSize_(0),
	fp_(0),
	transferDuration_(0),
	widgetLoadDuration_(0)
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
	deleteFile_(deleteFile),
	storageMode_(DiskStorage),
	data_(0),
	dataSize_(0),
	fp_(0),
	transferDuration_(0),
	widgetLoadDuration_(0)
{
}

VFile::VFile(bool deleteFile) :
	path_(VFile::tmpName()),
	deleteFile_(deleteFile),
	storageMode_(MemoryStorage),
	data_(0),
	dataSize_(0),
	fp_(0),
	transferDuration_(0),
	widgetLoadDuration_(0)
{
}

VFile::~VFile()
{
	close();

	if(data_)
		delete [] data_;

	if(deleteFile_)
	{
		//TODO: add further/better checks
		if(exists() && !path_.empty() && path_ != "/" && path_.size() > 4)
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

void VFile::setStorageMode(StorageMode mode)
{
	if(storageMode_ == mode)
		return;

	storageMode_=mode;

	if(storageMode_== DiskStorage)
	{
		if(dataSize_ > 0)
		{
			fp_ = fopen(path_.c_str(),"w");
			if(fwrite(data_,1,dataSize_,fp_) != dataSize_)
			{

			}
			fclose(fp_);
			fp_=NULL;
			delete [] data_;
			data_=0;
			dataSize_=0;
		}
	}
}

bool VFile::write(const char *buf,size_t len,std::string& err)
{

    //printf("total:%d \n len: %d \n",dataSize_,len);


	//Keep data in memory
	if(storageMode_ == MemoryStorage)
	{
		if(!data_)
		{
			data_ = new char[maxDataSize_+1];
		}

		if(dataSize_ + len  < maxDataSize_)
		{
			memcpy(data_+dataSize_,buf,len);
			dataSize_+=len;
			return true;
		}
		else
		{
			setStorageMode(DiskStorage);
		}
	}

	//Write data to disk
	if(storageMode_ == DiskStorage)
	{
		if(!fp_)
		{
			fp_ = fopen(path_.c_str(),"w");
		}

		if(fwrite(buf,1,len,fp_) != len)
		{
			//char buf_loc[2048];
		    //sprintf(buf_loc,"Write error on %s",out->path().c_str());
		    //gui::syserr(buf);
		    fclose(fp_);
		    return false;
		}
	}

	return true;
}

void VFile::close()
{
	if(fp_)
	{
		fclose(fp_);
		fp_=NULL;
	}
	if(data_)
	{
		data_[dataSize_]='\0';
		dataSize_++;
	}
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

//	char* s=std::string(tmpnam(NULL));
	res=std::string(tmpnam(NULL));

#endif

	return res;

}

