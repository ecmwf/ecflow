//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_OUTPUTDIRPROVIDER_HPP_
#define VIEWER_SRC_OUTPUTDIRPROVIDER_HPP_

#include <QObject>

#include "VDir.hpp"
#include "VInfo.hpp"
#include "InfoProvider.hpp"
#include "VTask.hpp"
#include "VTaskObserver.hpp"

#include "OutputFetchTask.hpp"

class OutputDirClient;
class OutputDirProvider;

class OutputDirFetchTask : public OutputFetchTask
{
public:
    OutputDirFetchTask(const std::string& name, OutputDirProvider* owner);
    void reset(ServerHandler* server,VNode* n,const std::string& filePath,RunCondition cond=NoCondition);

protected:
    OutputDirProvider* owner_{nullptr};
};

class OutputDirFetchRemoteTask : public QObject, public OutputDirFetchTask
{
Q_OBJECT
public:
    OutputDirFetchRemoteTask(OutputDirProvider* owner);
    ~OutputDirFetchRemoteTask();
    void run() override;
    void stop() override;
    void clear() override;

protected Q_SLOTS:
    void clientFinished();
    void clientProgress(QString,int);
    void clientError(QString);

protected:
    void deleteClient();

    OutputDirClient *client_{nullptr};
};


class OutputDirFetchLocalTask : public OutputDirFetchTask
{
public:
    OutputDirFetchLocalTask(OutputDirProvider* owner);
    void run() override;
};

class OutputDirProvider : public InfoProvider, public OutputFetchQueueOwner
{
    friend class OutputDirFetchRemoteTask;
    friend class OutputDirFetchLocalTask;

public:
     explicit OutputDirProvider(InfoPresenter* owner);
     ~OutputDirProvider();

	 void visit(VInfoNode*) override;
	 void clear() override;

     void fetchQueueSucceeded() override;
     void fetchQueueFinished(const std::string& filePath,VNode*) override;

private:
    OutputFetchQueue* fetchQueue_{nullptr};
    enum FetchTaskType {RemoteTask1, LocalTask1, RemoteTask2, LocalTask2};
    std::map<FetchTaskType, OutputDirFetchTask*> fetchTask_;
};


#endif /* VIEWER_SRC_OUTPUTDIRPROVIDER_HPP_ */
