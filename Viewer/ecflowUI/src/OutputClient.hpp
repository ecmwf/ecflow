//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_OUTPUTCLIENT_HPP_
#define VIEWER_SRC_OUTPUTCLIENT_HPP_

#include <QObject>
#include <QTcpSocket>
#include <QTime>

class QTcpSocket;

class OutputClient : public QObject
{
	Q_OBJECT

public:
    OutputClient(const std::string& host,const std::string& port,QObject *parent);
    ~OutputClient();

    const std::string& host() const {return host_;}
    int port() const {return port_;}
    const std::string& portStr() const {return portStr_;}

    const std::string& remoteFile() const {return remoteFile_;}
    bool ok() const { return soc_ != NULL; }

protected Q_SLOTS:
    virtual void slotError(QAbstractSocket::SocketError err)=0;
    virtual void slotRead()=0;
    virtual void slotConnected()=0;
    void slotCheckTimeout();

Q_SIGNALS:
	void error(QString);
    void progress(QString,int);
	void finished();

protected:
	void connectToHost(std::string,int);

	QTcpSocket* soc_;
	std::string host_;
	std::string portStr_;
	int port_;
	int timeout_;
	std::string remoteFile_;
	QTime stopper_;

private:
	OutputClient(const OutputClient&);
	OutputClient& operator=(const OutputClient&);
};


#endif /* VIEWER_SRC_OUTPUTCLIENT_HPP_ */
