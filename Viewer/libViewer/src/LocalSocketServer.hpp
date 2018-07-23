/***************************** LICENSE START ***********************************

 Copyright 2018 ECMWF and INPE. This software is distributed under the terms
 of the Apache License version 2.0. In applying this license, ECMWF does not
 waive the privileges and immunities granted to it by virtue of its status as
 an Intergovernmental Organization or submit itself to any jurisdiction.

 ***************************** LICENSE END *************************************/

#ifndef LOCALSOCKETSERVER_HPP
#define LOCALSOCKETSERVER_HPP

#include <QObject>

class QLocalServer;

class LocalSocketServer : public QObject
{
    Q_OBJECT

public:
    LocalSocketServer(QString serverId,QObject *parent);
    ~LocalSocketServer();

    static QString generateServerName(QString serverId,qint64 pid);
    QString serverName();

protected Q_SLOTS:
    void slotMessageReceived();

Q_SIGNALS:
    void messageReceived(QString);

protected:
    QString generateServerName();

    QLocalServer* server_;
    QString serverId_;
};

#endif // LOCALSOCKETSERVER_HPP
