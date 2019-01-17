//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VIEWER_SRC_OUTPUTDIRCLIENT_HPP_
#define VIEWER_SRC_OUTPUTDIRCLIENT_HPP_

#include "OutputClient.hpp"
#include "VDir.hpp"

#include <QByteArray>

class OutputDirClient : public OutputClient
{
	Q_OBJECT

public:
    OutputDirClient(const std::string& host,const std::string& port,QObject *parent);

    VDir_ptr result() const;
    void getDir(const std::string& name);

protected Q_SLOTS:
    void slotError(QAbstractSocket::SocketError err);
    void slotRead();
    void slotConnected();
    void slotCheckTimeout();

private:
	OutputDirClient(const OutputDirClient&);
	OutputDirClient& operator=(const OutputDirClient&);

	void parseData();

	VDir_ptr  dir_;
	QByteArray data_;
};


#endif /* VIEWER_SRC_OUTPUTDIRCLIENT_HPP_ */
