//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_OUTPUTFILECLIENT_HPP_
#define VIEWER_SRC_OUTPUTFILECLIENT_HPP_

#include "OutputClient.hpp"
#include "VFile.hpp"

class OutputFileClient : public OutputClient
{
	Q_OBJECT

public:
    OutputFileClient(const std::string& host,const std::string& port,QObject *parent);

    const std::string& remoteFile() const {return remoteFile_;}
    VFile_ptr result() const;
    void getFile(const std::string& name);

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err);
    void slotRead();
    void slotConnected();

private:
	OutputFileClient(const OutputClient&);
	OutputFileClient& operator=(const OutputClient&);

	std::string remoteFile_;
	qint64 total_;
	VFile_ptr out_;
	qint64 lastProgress_;
	const QString progressUnits_;
	const qint64 progressChunk_;

};

#endif /* VIEWER_SRC_OUTPUTFILECLIENT_HPP_ */
