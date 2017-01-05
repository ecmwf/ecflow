//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogTruncator.hpp"

#include <QFileInfo>
#include <QTimer>

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"

LogTruncator::LogTruncator(QString path, int timeout,int sizeLimit,int lineNum,QObject* parent) :
    QObject(parent),
    path_(path),
    timeout_(timeout),
    sizeLimit_(sizeLimit),
    lineNum_(lineNum)
{
    timer_=new QTimer(this);
    connect(timer_,SIGNAL(timeout()),this,SLOT(truncate()));
    timer_->start(timeout_);
}

void LogTruncator::truncate()
{
    //The log file might not exist. It is not necessarily an error because
    //the startup script decides on whether the main log goes into the stdout or into
    //a file.
    QFileInfo info(path_);
    if(!info.exists())
        return;

    Q_EMIT truncateBegin();

    qint64 fSize=info.size();
    if(fSize > sizeLimit_)
    {
        std::string errStr;
        DirectoryHandler::truncateFile(path_.toStdString(),
                          lineNum_,errStr);

        info.refresh();
        UiLog().info() << "LogTruncator::truncate --> truncated from " << fSize <<
                          " bytes to " <<  info.size() << " bytes;  log file: " << path_;
    }

    Q_EMIT truncateEnd();
}
