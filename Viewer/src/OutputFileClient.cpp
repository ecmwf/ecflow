//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputFileClient.hpp"

#include "UiLog.hpp"

#define _UI_OUTPUTFILECLIENT_DEBUG

OutputFileClient::OutputFileClient(const std::string& host,const std::string& portStr,QObject* parent) :
	OutputClient(host,portStr,parent),
	total_(0),
    expected_(0),
	lastProgress_(0),
	progressChunk_(1024*1024),
	progressUnits_("MB")
{
}

void OutputFileClient::clearResult()
{
    if(out_)
    {
        out_->close();
        out_.reset();
    }
}

void OutputFileClient::slotConnected()
{
	soc_->write("get ",4);
	soc_->write(remoteFile_.c_str(),remoteFile_.size());
	soc_->write("\n",1);
}

void OutputFileClient::slotError(QAbstractSocket::SocketError err)
{
    switch(err)
	{
	case QAbstractSocket::RemoteHostClosedError:

		if(total_ == 0)
		{
            break;
		}
        //Probably there was no error and the connection was closed because all
        //the data was transferred. Unfortunately the logserver does not send anything
        //at the end of the data transfer.
        else
		{
            soc_->abort();
            if(out_)
			{
				out_->setTransferDuration(stopper_.elapsed());
				out_->setFetchDate(QDateTime::currentDateTime());
				out_->close();
			}

			Q_EMIT finished();

			if(out_)
				out_.reset();

            return;

		}

		break;
    case QAbstractSocket::UnknownSocketError:
        if(soc_->state() != QAbstractSocket::ConnectedState)
        {
            break;
        }
        break;
	default:		
		break;
	}

    soc_->abort();
    if(out_)
    {
        out_->close();
        out_.reset();
    }
    Q_EMIT error(soc_->errorString());

}

void OutputFileClient::getFile(const std::string& name)
{
	connectToHost(host_,port_);

	remoteFile_=name;
	out_.reset();
    out_=VFile_ptr(VFile::create(true)); //we will delete the file from disk
    out_->setSourcePath(name);
    total_=0;
    lastProgress_=0;
    estimateExpectedSize();
}

void OutputFileClient::slotRead()
{
	const qint64 size = 64*1024;
	char buf[size];
	quint64 len = 0;

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

            int prog=0;
            if(expected_ > 0)
            {
                prog=static_cast<int>(100.*static_cast<float>(total_)/static_cast<float>(expected_));
                if(prog>100) prog=100;
                Q_EMIT progress(QString::number(lastProgress_) + "/" + QString::number(expected_/progressChunk_) +
                               " " + progressUnits_  + " transferred",prog);
            }
            else
                Q_EMIT progress(QString::number(lastProgress_) + " " + progressUnits_ + " transferred",prog);
		}
	}
}

VFile_ptr OutputFileClient::result() const
{
	return out_;
}

void OutputFileClient::setExpectedSize(qint64 v)
{
    expected_=v;
}

int OutputFileClient::maxProgress() const
{
    return expected_/progressChunk_;
}

void OutputFileClient::setDir(VDir_ptr dir)
{
    if(dir != dir_)
    {
        dir_=dir;
        if(expected_ == 0)
            estimateExpectedSize();
    }
}

void OutputFileClient::estimateExpectedSize()
{
    if(!dir_)
    {
        expected_=0;
        return;
    }

#ifdef _UI_OUTPUTFILECLIENT_DEBUG
    UiLog().dbg() << "OutputFileClient::estimateExpectedSize -->";
#endif
    for(unsigned int i=0; i < dir_->count(); i++)
    {
#ifdef _UI_OUTPUTFILECLIENT_DEBUG
        UiLog().dbg() << "file: " << dir_->fullName(i);
#endif
        if(dir_->fullName(i) == remoteFile_)
        {
            expected_=dir_->items().at(i)->size_;
#ifdef _UI_OUTPUTFILECLIENT_DEBUG
            UiLog().dbg() << "  expected size=" << expected_;
#endif
            return;
        }
    }

    expected_=0;
#ifdef _UI_OUTPUTFILECLIENT_DEBUG
    UiLog().dbg() << "  expected size=" << QString::number(expected_);
#endif
}
