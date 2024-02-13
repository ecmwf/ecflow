/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_LocalSocketServer_HPP
#define ecflow_viewer_LocalSocketServer_HPP

#include <QObject>

class QLocalServer;

class LocalSocketServer : public QObject {
    Q_OBJECT

public:
    LocalSocketServer(QString serverId, QObject* parent);
    ~LocalSocketServer() override;

    static QString generateServerName(QString serverId, qint64 pid);
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

#endif /* ecflow_viewer_LocalSocketServer_HPP */
