//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogViewerProc.hpp"

#include <QLocalSocket>

#include "LocalSocketServer.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"

LogViewerProc::LogViewerProc() :
    QObject(0),
    socket_(NULL)
{
    if(char* logexe=getenv("ECFLOWUI_LOG_VIEWER"))
    {
        program_=QString(logexe);
        //QStringList arguments;
        //arguments << "-style" << "fusion";

        if(!program_.isEmpty())
            proc_ = new QProcess(this);
        //logProc_->start(program, arguments);
    }
}

void LogViewerProc::addToWin(ServerHandler* sh)
{
    if(!proc_)
        return;

    if(sh)
    {
        QStringList args;
        args << QString::fromStdString(sh->name()) <<
                QString::fromStdString(sh->host()) <<
                QString::fromStdString(sh->port());

        QString logFile;
        if(VServer* vs=sh->vRoot())
        {
            logFile=QString::fromStdString(vs->findVariable("ECF_LOG",false));
        }
        args << logFile;

        start(args);
    }
}

void LogViewerProc::start(QStringList args)
{
    if(logViewerId_.isEmpty())
    {
        qint64 pid;
        if(QProcess::startDetached(program_,args,QString(),&pid))
        {
            logViewerId_ = LocalSocketServer::generateServerName("log",pid);
        }
    }
    else
    {
        if(!socket_)
        {
            socket_ = new QLocalSocket(this);
            socket_->setServerName(logViewerId_);
            socket_->connectToServer(QIODevice::WriteOnly);
            if(socket_->waitForConnected(1000))
            {
                socket_->write("hello");
            }

            //connect(socket_,SIGNAL(connected()),
            //        this,SLOT(slotConnected()));
        }
    }

#if 0
    Q_ASSERT(proc_);



    qint64 pid=0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    pid=proc_->processId();
#endif



    //already running
    if(pid > 0)
    {
        proc_->write("hello");
        proc_->closeWriteChannel();
    }
    else
    {
        proc_->start(program_, args);
    }
#endif

}
