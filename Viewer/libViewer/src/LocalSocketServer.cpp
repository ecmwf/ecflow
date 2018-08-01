//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "LocalSocketServer.hpp"

#include <QtGlobal>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>
#include "DirectoryHandler.hpp"

#include <unistd.h>

LocalSocketServer::LocalSocketServer(QString serverId,QObject* parent) :
    QObject(parent),
    serverId_(serverId)
{
    QString name=generateServerName();

    //remove existings sockets with the same name
    QLocalServer::removeServer(name);

    //Create the server
    server_ = new QLocalServer(parent);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    //Restrict access to the socket
    server_->setSocketOptions(QLocalServer::UserAccessOption);
#endif

    //Start listening
    bool b=server_->listen(name);

    qDebug() << "b" << b << server_->serverError() << server_->errorString();
    qDebug() << "full" << server_->fullServerName();

    connect(server_,SIGNAL(newConnection()),
        this,SLOT(slotMessageReceived()));
}

LocalSocketServer::~LocalSocketServer()
{
}

void LocalSocketServer::slotMessageReceived()
{
    QLocalSocket *localSocket = server_->nextPendingConnection();
    if(!localSocket->waitForReadyRead(5000))
    {
        //qDebug(localSocket->errorString().toLatin1());
        return;
    }
    QByteArray byteArray = localSocket->readAll();
    QString message = QString::fromUtf8(byteArray.constData());
    Q_EMIT messageReceived(message);
}

QString LocalSocketServer::serverName()
{
    return generateServerName();
}

QString LocalSocketServer::generateServerName()
{
    return generateServerName(serverId_,qApp->applicationPid());
}

QString LocalSocketServer::generateServerName(QString serverId,qint64 pid)
{
    std::string socketPath=DirectoryHandler::socketDir();
    if(!socketPath.empty())
    {
        return QString::fromStdString(socketPath) + "/" + serverId + "_" + QString::number(pid);
    }

    return QString();
}
