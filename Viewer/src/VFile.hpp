//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VFILE_INC__
#define VFILE_INC__

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <QDateTime>

class VFile;
typedef boost::shared_ptr<VFile> VFile_ptr;

class VFile : public boost::enable_shared_from_this<VFile>
{
public:
	virtual ~VFile();

	enum StorageMode {MemoryStorage,DiskStorage};

	const std::string& path() const {return path_;}
	void  setContents(const std::string);
	bool  exists() const;

	StorageMode storageMode() const {return storageMode_;}
	static const size_t maxDataSize() {return maxDataSize_;}
	size_t dataSize() const {return dataSize_;}
	const char* data() const {return data_;}

	void setTransferDuration(unsigned int  d) {transferDuration_=d;}
	unsigned int transferDuration() const {return transferDuration_;}
	void setFetchDate(QDateTime d) {fetchDate_=d;}
	QDateTime fetchDate() const {return fetchDate_;}
	void setWidgetLoadDuration(unsigned int t) {widgetLoadDuration_=t;}
	unsigned int widgetLoadDuration() const {return widgetLoadDuration_;}

	bool write(const char *buf,size_t len,std::string& err);
	void close();

	static VFile_ptr create(const std::string& path,const std::string& contents,bool deleteFile=true);
	static VFile_ptr create(const std::string& path,bool deleteFile= true);
	static VFile_ptr create(bool deleteFile= true);

	static std::string tmpName();

protected:
	VFile(const std::string& name,const std::string& str,bool deleteFile=true);
	VFile(const std::string& str,bool deleteFile= true);
	explicit VFile(bool deleteFile= true);
	void setStorageMode(StorageMode);

	std::string path_;
	bool  deleteFile_;

	StorageMode storageMode_;
	static const size_t maxDataSize_;
	char* data_;
	size_t dataSize_;
	FILE* fp_;
	unsigned int transferDuration_;
	QDateTime fetchDate_;
	unsigned int widgetLoadDuration_;
};

#endif
