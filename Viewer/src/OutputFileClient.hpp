//============================================================================
// Copyright 2009-2017 ECMWF.
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
#include "VDir.hpp"
#include "VFile.hpp"

class OutputFileClient : public OutputClient
{
	Q_OBJECT

public:
    OutputFileClient(const std::string& host,const std::string& port,QObject *parent);

    VFile_ptr result() const;
    void clearResult();
    void getFile(const std::string& name);
    void setExpectedSize(qint64 v);
    int maxProgress() const;
    void setDir(VDir_ptr);

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err);
    void slotRead();
    void slotConnected();

private:
	OutputFileClient(const OutputClient&);
	OutputFileClient& operator=(const OutputClient&);
    void estimateExpectedSize();

	qint64 total_;
    qint64 expected_;
	VFile_ptr out_;
    VDir_ptr dir_;
	qint64 lastProgress_;
	const QString progressUnits_;
	const qint64 progressChunk_;
};

#endif /* VIEWER_SRC_OUTPUTFILECLIENT_HPP_ */
