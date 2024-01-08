/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_OutputFileProvider_HPP
#define ecflow_viewer_OutputFileProvider_HPP

#include <map>

#include <QObject>

#include "FetchTask.hpp"
#include "GenFileProvider.hpp"
#include "InfoProvider.hpp"
#include "VDir.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class OutputFileProvider;
class OutputFileFetchQueueManager;
class OutputCache;
struct OutputCacheItem;

class OutputFileFetchServerTask : public AbstractFetchTask {
public:
    explicit OutputFileFetchServerTask(FetchQueueOwner* owner);
    void run() override;
    void setFileProvider(OutputFileProvider* p) { fileProvider_ = p; }

protected:
    OutputFileProvider* fileProvider_{nullptr};
};

class OutputFileProvider : public QObject,
                           public InfoProvider //, public OutputFetchQueueOwner
{
    friend class OutputFileFetchServerTask;
    friend class OutputFileFetchQueueManager;

public:
    explicit OutputFileProvider(InfoPresenter* owner);
    ~OutputFileProvider();

    void visit(VInfoNode*) override;
    void clear() override;

    // Get a particular jobout file
    void fetchCurrentJobout(bool useCache);
    void fetchFile(const std::string& fileName, size_t deltaPos, bool useCache);
    void fetchFileForMode(VFile_ptr f, size_t deltaPos, bool useCache);
    void fetchFileForMode(const std::string& fileName, VDir::FetchMode fetchMode, bool useCache);
    void setDirectories(const std::vector<VDir_ptr>&);

    std::string joboutFileName() const;
    bool isTryNoZero(const std::string& fileName) const;

protected:
    OutputCacheItem* findInCache(const std::string& fileName);
    void addToCache(VFile_ptr file);
    void fetchJoboutViaServer(ServerHandler* server, VNode* n, const std::string&);
    VDir_ptr dirToFile(const std::string& fileName) const;

private:
    void fetchFileInternal(ServerHandler* server,
                           VNode* n,
                           const std::string& fileName,
                           bool isJobout,
                           size_t deltaPos,
                           bool detachCache);
    void fetchFileForModeInternal(const std::string& fileName,
                                  VFile::FetchMode fetchMode,
                                  size_t deltaPos,
                                  unsigned int modTime,
                                  const std::string& checkSum,
                                  bool useCache);

    OutputCache* outCache_{nullptr};
    OutputFileFetchQueueManager* fetchManager_{nullptr};
    std::vector<VDir_ptr> dirs_;
};

#endif /* ecflow_viewer_OutputFileProvider_HPP */
