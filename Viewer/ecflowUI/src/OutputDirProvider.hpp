/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_OutputDirProvider_HPP
#define ecflow_viewer_OutputDirProvider_HPP

#include <QObject>

#include "FetchTask.hpp"
#include "InfoProvider.hpp"
#include "VDir.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

class OutputDirClient;
class OutputDirProvider;
class OutputDirFetchQueueManager;
class VDirTransfer;

class OutputDirFetchTask : public AbstractFetchTask {
public:
    OutputDirFetchTask(const std::string& name, FetchQueueOwner* owner);

protected:
    void addTryLog(VReply* r, const std::string& txt) const;
};

class OutputDirFetchLogServerTask : public QObject, public OutputDirFetchTask {
    Q_OBJECT
public:
    OutputDirFetchLogServerTask(FetchQueueOwner* owner);
    ~OutputDirFetchLogServerTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void clientFinished();
    void clientProgress(QString, int);
    void clientError(QString);

protected:
    void deleteClient();

    OutputDirClient* client_{nullptr};
};

class OutputDirFetchLocalTask : public OutputDirFetchTask {
public:
    OutputDirFetchLocalTask(FetchQueueOwner* owner);
    void run() override;
};

class OutputDirFetchTransferTask : public QObject, public OutputDirFetchTask {
    Q_OBJECT
public:
    OutputDirFetchTransferTask(FetchQueueOwner* owner);
    ~OutputDirFetchTransferTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void transferFinished();
    void transferProgress(QString, int);
    void transferFailed(QString);

protected:
    void stopTransfer();
    void parseLine(QString line);

    VDirTransfer* transfer_{nullptr};
    VDir_ptr dir_;
};

class OutputDirProvider : public InfoProvider //, public OutputFetchQueueOwner
{
    friend class OutputDirFetchQueueManager;

public:
    explicit OutputDirProvider(InfoPresenter* owner);
    ~OutputDirProvider();
    OutputDirProvider(const OutputDirProvider&)            = delete;
    OutputDirProvider& operator=(const OutputDirProvider&) = delete;

    void visit(VInfoNode*) override;
    void clear() override;

private:
    OutputDirFetchQueueManager* fetchManager_{nullptr};
};

#endif /* ecflow_viewer_OutputDirProvider_HPP */
