/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_OutputClient_HPP
#define ecflow_viewer_OutputClient_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QTcpSocket>

class QTcpSocket;

class OutputClient : public QObject {
    Q_OBJECT

public:
    OutputClient(const std::string& host, const std::string& port, QObject* parent);
    ~OutputClient() override;

    const std::string& host() const { return host_; }
    int port() const { return port_; }
    const std::string& portStr() const { return portStr_; }
    const std::string& remoteFile() const { return remoteFile_; }
    bool ok() const { return soc_ != nullptr; }
    std::string longName() const;

protected Q_SLOTS:
    virtual void slotError(QAbstractSocket::SocketError err) = 0;
    virtual void slotRead()                                  = 0;
    virtual void slotConnected()                             = 0;
    void slotCheckTimeout();

Q_SIGNALS:
    void error(QString);
    void progress(QString, int);
    void finished();

protected:
    void connectToHost(std::string, int);
    virtual void timeoutError() {}

    QTcpSocket* soc_;
    std::string host_;
    std::string portStr_;
    int port_;
    int timeout_;
    std::string remoteFile_;
    QElapsedTimer stopper_;

private:
    OutputClient(const OutputClient&);
    OutputClient& operator=(const OutputClient&);
};

#endif /* ecflow_viewer_OutputClient_HPP */
