/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
