/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_FileFetchTask_HPP
#define ecflow_viewer_FileFetchTask_HPP

#include <deque>
#include <map>

#include <QObject>

#include "FetchTask.hpp"
#include "VFile.hpp"

class FileProvider;
class OutputFileClient;
class VFileTransfer;
class VReply;

class FileFetchLocalTask : public AbstractFetchTask {
public:
    explicit FileFetchLocalTask(FetchQueueOwner* owner);
    void run() override;
};

class FileFetchTransferTask : public QObject, public AbstractFetchTask {
    Q_OBJECT
public:
    explicit FileFetchTransferTask(FetchQueueOwner* owner);
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void transferFinished();
    void transferProgress(QString, int);
    void transferFailed(QString);

protected:
    void stopTransfer();

    VFileTransfer* transfer_{nullptr};
};

class FileFetchCacheTask : public AbstractFetchTask {
public:
    explicit FileFetchCacheTask(FetchQueueOwner* owner);
    void run() override;
};

class FileFetchLogServerTask : public QObject, public AbstractFetchTask {
    Q_OBJECT
public:
    explicit FileFetchLogServerTask(FetchQueueOwner* owner);
    ~FileFetchLogServerTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void clientFinished();
    void clientProgress(QString, int);
    void clientError(QString);

protected:
    void deleteClient();

    OutputFileClient* client_{nullptr};
};

#endif /* ecflow_viewer_FileFetchTask_HPP */
