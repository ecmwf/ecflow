//============================================================================
// Copyright 2009-2019 ECMWF.
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

class OutputDirClient;

class OutputDirProviderTask
{
public:
    enum FetchMode {LocalFetch,RemoteFetch};
    enum Status {UnkownStatus,FinishedStatus,FailedStatus};
    enum Condition {NoCondition,RunIfPrevFailed};

    OutputDirProviderTask(const std::string& path,FetchMode fetchMode,Condition cond=NoCondition) :
        path_(path), fetchMode_(fetchMode), condition_(cond), status_(UnkownStatus) {}

    std::string path_;
    VDir_ptr dir_;
    QString error_;
    FetchMode fetchMode_;
    Condition condition_;
    Status status_;
};


class OutputDirProvider : public QObject, public InfoProvider
{
Q_OBJECT

public:
	 explicit OutputDirProvider(InfoPresenter* owner);

	 void visit(VInfoNode*) override;
	 void clear() override;

private Q_SLOTS:
	void slotOutputClientError(QString);
    void slotOutputClientProgress(QString,int);
	void slotOutputClientFinished();

private:
    bool hasNext() const;
    void fetchNext();
    void fetchIgnored();
    void fetchFinished(VDir_ptr,QString msg=QString());
    void fetchFailed(QString msg=QString());
    void failed(QString);
    void completed();

	bool fetchDirViaOutputClient(VNode *n,const std::string& fileName);
    VDir_ptr fetchLocalDir(const std::string& path,std::string& errorStr);
	OutputDirClient* makeOutputClient(const std::string& host,const std::string& port);

	OutputDirClient *outClient_;
    QList<OutputDirProviderTask> queue_;
    int currentTask_;
};


#endif /* VIEWER_SRC_OUTPUTDIRPROVIDER_HPP_ */
