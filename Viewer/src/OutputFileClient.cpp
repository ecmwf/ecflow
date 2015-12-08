//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputFileClient.hpp"

OutputFileClient::OutputFileClient(const std::string& host,const std::string& portStr,QObject* parent) :
	OutputClient(host,portStr,parent),
	total_(0),
	lastProgress_(0),
	progressChunk_(1024*1024),
	progressUnits_("MB")
{
}

void OutputFileClient::slotConnected()
{
	qDebug() << "connected to " << soc_->peerName();

	Q_EMIT progress("");

	soc_->write("get ",4);
	soc_->write(remoteFile_.c_str(),remoteFile_.size());
	soc_->write("\n",1);
}

void OutputFileClient::slotError(QAbstractSocket::SocketError err)
{
	qDebug() << "file error" <<  soc_->errorString();

	switch(err)
	{
	case QAbstractSocket::RemoteHostClosedError:
		qDebug() << "remote host closed";

		if(out_)
		{
			out_->setTransferDuration(stopper_.elapsed());
			out_->setFetchDate(QDateTime::currentDateTime());
			out_->close();
		}

		Q_EMIT finished();

		if(out_)
			out_.reset();

		break;
	default:
		soc_->abort();
		if(out_)
		{
			out_->close();
			out_.reset();
		}
		Q_EMIT error(soc_->errorString());
		break;
	}
}

void OutputFileClient::getFile(const std::string& name)
{
	connectToHost(host_,port_);

	remoteFile_=name;
	out_.reset();
	out_=VFile_ptr(VFile::create(false));
	total_=0;
}

void OutputFileClient::slotRead()
{
	const qint64 size = 64*1024;
	char buf[size];
	quint64 len = 0;

	//if(soc_->bytesAvailable() < (int)sizeof(quint16))
	//	return;

	while((len = soc_->read(buf,size)) > 0)
	{
		std::string err;
		if(!out_->write(buf,len,err))
		{
			soc_->abort();
			Q_EMIT error("write failed");
		}
		total_ += len;

		if(total_/progressChunk_ > lastProgress_)
		{
			lastProgress_=total_/progressChunk_;
			Q_EMIT progress(QString::number(lastProgress_) + " " + progressUnits_ + " read");
			qDebug() << "progress";

		}

	}

	//qDebug() << "file total" << total_;
}

VFile_ptr OutputFileClient::result() const
{
	return out_;
}



