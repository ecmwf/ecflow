//============================================================================
// Copyright 2014 ECMWF.
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

#include "VFile.hpp"
#include "VDir.hpp"

class QTcpSocket;
class OutputClient;
typedef boost::shared_ptr<OutputClient> LogClient_ptr;

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
    VFile_ptr resultFile() const;
    void getFile(const std::string& name);
    //VFile_ptr getFile(std::string name);
    //VDir_ptr  getDir(const char* name);
    bool ok() const { return soc_ != NULL; }

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err);
    void slotRead();
    void slotConnected();
    void slotCheckTimeout();

Q_SIGNALS:
	void error(QString);
	void progress(QString);
	void finished();

private:
	OutputClient(const OutputClient&);
	OutputClient& operator=(const OutputClient&);
	void connectToHost(std::string,int);

	QTcpSocket* soc_;
	std::string host_;
	std::string portStr_;
	int port_;
	int timeout_;
	std::string remoteFile_;
	qint64 total_;
	VFile_ptr out_;
	QTime stopper_;
	qint64 lastProgress_;
	const QString progressUnits_;
	const qint64 progressChunk_;

};


#endif /* VIEWER_SRC_OUTPUTCLIENT_HPP_ */
