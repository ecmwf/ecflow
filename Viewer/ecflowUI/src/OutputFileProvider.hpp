//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef OUTPUTFILEPROVIDER_HPP_
#define OUTPUTFILEPROVIDER_HPP_

#include <map>

#include <QObject>

#include "VFile.hpp"
#include "VDir.hpp"
#include "VInfo.hpp"
#include "InfoProvider.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

#include "OutputFetchTask.hpp"

class OutputFileProvider;
class OutputFileClient;
class OutputCache;
struct OutputCacheItem;
class VFileTransfer;

class OutputFileFetchTask : public OutputFetchTask
{
public:
    OutputFileFetchTask(const std::string& name, OutputFileProvider* owner);
    void reset(ServerHandler* server,VNode* node,const std::string& filePath, bool isJobout,
               size_t deltaPos, bool useCache);
    void clear() override;

protected:
    OutputFileProvider* owner_{nullptr};
    bool isJobout_{false};
    size_t deltaPos_{0};
    bool useCache_{false};
};

class OutputFileFetchCacheTask : public OutputFileFetchTask
{
public:
    OutputFileFetchCacheTask(OutputFileProvider* owner);
    void run() override;
};


class OutputFileFetchRemoteTask : public QObject, public OutputFileFetchTask
{
Q_OBJECT
public:
    OutputFileFetchRemoteTask(OutputFileProvider* owner);
    ~OutputFileFetchRemoteTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void clientFinished();
    void clientProgress(QString,int);
    void clientError(QString);

protected:
    void deleteClient();

    OutputFileClient *client_{nullptr};
};

class OutputFileFetchAnyLocalTask : public OutputFileFetchTask
{
public:
    OutputFileFetchAnyLocalTask(OutputFileProvider* owner);
    void run() override;
};

class OutputFileFetchLocalTask : public OutputFileFetchAnyLocalTask
{
public:
    OutputFileFetchLocalTask(OutputFileProvider* owner);
    void run() override;
};


class OutputFileFetchTransferTask : public QObject, public OutputFileFetchTask
{
Q_OBJECT
public:
    OutputFileFetchTransferTask(OutputFileProvider* owner);
    ~OutputFileFetchTransferTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void transferFinished();
    void transferProgress(QString,int);
    void transferFailed(QString);

protected:
    void stopTransfer();

    VFileTransfer *transfer_{nullptr};
    VFile_ptr resFile_;
};

class OutputFileFetchServerTask : public OutputFileFetchTask
{
public:
    OutputFileFetchServerTask(OutputFileProvider* owner);
    void run() override;
};

class OutputFileProvider : public QObject, public InfoProvider, public OutputFetchQueueOwner
{
     //friend class OutputFileTask;
     friend class OutputFileFetchCacheTask;
     friend class OutputFileFetchRemoteTask;
     friend class OutputFileFetchLocalTask;
     friend class OutputFileFetchAnyLocalTask;
     friend class OutputFileFetchTransferTask;
     friend class OutputFileFetchServerTask;

public:
	 explicit OutputFileProvider(InfoPresenter* owner);
     ~OutputFileProvider();

	 void visit(VInfoNode*) override;
	 void clear() override;

	 //Get a particular jobout file
     void file(const std::string& fileName,size_t deltaPos, bool useCache);
     void fetchFile(const std::string& fileName,VFile::FetchMode fetchMode,size_t deltaPos, bool useCache);
     void fetchFile(const std::string& fileName,VDir::FetchMode fetchMode,size_t deltaPos, bool useCache);
     void setDirectories(const std::vector<VDir_ptr>&);

     std::string joboutFileName() const;
     bool isTryNoZero(const std::string& fileName) const;

     void fetchQueueSucceeded() override;
     void fetchQueueFinished(const std::string& filePath, VNode*) override;

protected:
     OutputCacheItem* findInCache(const std::string& fileName);
     void addToCache(VFile_ptr file);
     void fetchJoboutViaServer(ServerHandler *server,VNode *n,const std::string&);
     VDir_ptr dirToFile(const std::string& fileName) const;

private:
     void fetchFile(ServerHandler *server,VNode *n,const std::string& fileName,bool isJobout, size_t deltaPos,bool detachCache);

     OutputCache* outCache_{nullptr};
     OutputFetchQueue* fetchQueue_{nullptr};
     std::vector<VDir_ptr> dirs_;
     enum FetchTaskType {RemoteTask, LocalTask, AnyLocalTask, CacheTask, ServerTask, TransferTask};
     std::map<FetchTaskType, OutputFileFetchTask*> fetchTask_;
};

#endif
