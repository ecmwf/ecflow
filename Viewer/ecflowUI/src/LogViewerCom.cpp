//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LogViewerCom.hpp"

#include <QLocalSocket>
#include <QProcess>

#include "LocalSocketServer.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"

LogViewerCom::LogViewerCom() :
    QObject(nullptr)
{
    if(char* logexe=getenv("ECFLOWUI_LOG_VIEWER"))
    {
        program_=QString(logexe);       
    }
}

void LogViewerCom::closeApp()
{
    if(!logViewerId_.isEmpty())
    {
        auto* socket = new QLocalSocket(this);
        socket->setServerName(logViewerId_);

        socket->connectToServer(QIODevice::WriteOnly);
        if(socket->waitForConnected(1000))
        {
            socket->write("exit");
            socket->disconnectFromServer();
            socket->waitForDisconnected(1000);
        }
    }
}

void LogViewerCom::addToApp(ServerHandler* sh)
{
    if(program_.isEmpty())
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

void LogViewerCom::start(QStringList args)
{
    if(program_.isEmpty())
        return;

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
        //Send message over local socket
        auto* socket = new QLocalSocket(this);
        socket->setServerName(logViewerId_);
        socket->connectToServer(QIODevice::WriteOnly);

        if(socket->waitForConnected(1000))
        {
            socket->write(args.join("::").toUtf8());
            socket->disconnectFromServer();
            socket->waitForDisconnected(1000);
        }
        //no connection: proc probably stopped
        else
        {
            //start new process
            qint64 pid;
            if(QProcess::startDetached(program_,args,QString(),&pid))
            {
                logViewerId_ = LocalSocketServer::generateServerName("log",pid);
            }
            else
            {
                logViewerId_.clear();
            }
        }

        delete socket;
    }
}


