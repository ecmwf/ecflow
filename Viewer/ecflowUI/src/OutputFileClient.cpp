//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputFileClient.hpp"

#include "UiLog.hpp"

#define UI_OUTPUTFILECLIENT_DEBUG__
#define UI_OUTPUTFILECLIENT_DETAILED_DEBUG__

OutputFileClient::OutputFileClient(const std::string& host,const std::string& portStr,QObject* parent) :
    OutputClient(host,portStr,parent)
{
    versionClient_ = new OutputVersionClient(host, portStr, parent);
    connect(versionClient_,SIGNAL(finished()), this, SLOT(slotVersionFinished()));
    connect(versionClient_,SIGNAL(error(QString)), this, SLOT(slotVersionError(QString)));
}

void OutputFileClient::clearResult()
{
    if(out_) {
        out_->close();
        out_.reset();
    }
}

void OutputFileClient::slotConnected()
{ 
    if (deltaPos_ > 0) {
        soc_->write("delta ",6);
        std::string s = std::to_string(deltaPos_) + " ";
        soc_->write(s.c_str(), s.size());
    } else  {
        soc_->write("get ",4);
    }
    soc_->write(remoteFile_.c_str(),remoteFile_.size());

    soc_->write("\n",1);
}

void OutputFileClient::slotError(QAbstractSocket::SocketError err)
{
#ifdef UI_OUTPUTFILECLIENT_DEBUG__
    UiLog().dbg() <<  UI_FN_INFO  << " --> " << soc_->errorString();
#endif

    switch(err)
	{
	case QAbstractSocket::RemoteHostClosedError:

        if(total_ == 0 && deltaPos_ <=0)
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
                out_->print();
                out_->close();
			}

			Q_EMIT finished();

            if(out_) {
				out_.reset();
            }
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

void OutputFileClient::getFile(const std::string& name, size_t deltaPos)
{
    // stop running the current transfer
    soc_->abort();
    if(out_)
    {
        out_->close();
        out_.reset();
    }

    remoteFile_=name;
    deltaPos_ = deltaPos;

    // we need to know the version. When it finishes will call getFile again
    if (versionClient_->versionStatus() == OutputVersionClient::VersionNotFetched) {
#ifdef UI_OUTPUTFILECLIENT_DEBUG__
        UiLog().dbg() << "OutputFileClient::getFile --> Try to get logserver version";
#endif
        versionClient_->getVersion();
        return;
    }

    // only verson 2 can handle delta increments
    if (versionClient_->version() != 2) {
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
        UiLog().dbg() << "OutputFileClient::getFile --> deltaPos set to 0";
#endif
        deltaPos_ = 0;
    }

	remoteFile_=name;
	out_.reset();
    out_=VFile_ptr(VFile::create(true)); //we will delete the file from disk
    out_->setDeltaContents(deltaPos_ >0);
    out_->setSourcePath(name);
    total_=0;
    lastProgress_=0;
    estimateExpectedSize();

    connectToHost(host_,port_);
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
    if(dir_ && dir && dir_.get() == dir.get())
        return;

    dir_=dir;
    estimateExpectedSize();
}

void OutputFileClient::estimateExpectedSize()
{
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
    UI_FN_DBG
#endif
    if(!dir_)
    {
        expected_=0;
        return;
    }

    for(int i=0; i < dir_->count(); i++)
    {
//#ifdef _UI_OUTPUTFILECLIENT_DETAILED_DEBUG
//        UiLog().dbg() << " file: " << dir_->fullName(i);
//#endif
        if(dir_->fullName(i) == remoteFile_)
        {
            expected_=dir_->items().at(i)->size_;
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
            UiLog().dbg() << " expected size=" << expected_;
#endif
            return;
        }
    }

    expected_=0;
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
    UiLog().dbg() << " expected size=" << QString::number(expected_);
#endif
}

void OutputFileClient::slotVersionFinished()
{
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
    UI_FN_DBG
#endif
    Q_ASSERT(versionClient_->versionStatus() != OutputVersionClient::VersionNotFetched);
    getFile(remoteFile_, deltaPos_);
}

void OutputFileClient::slotVersionError(QString errorText)
{
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "errorText=" << errorText;
#endif
    Q_ASSERT(versionClient_->versionStatus() != OutputVersionClient::VersionNotFetched);
    getFile(remoteFile_, deltaPos_);
}

//===================================
//
// OutputVersionClient
//
//===================================

void OutputVersionClient::timeoutError()
{
    versionStatus_ = VersionFailedToFetch;
}

void OutputVersionClient::slotConnected()
{
    soc_->write("version",7);
    soc_->write("\n",1);
}

void OutputVersionClient::slotError(QAbstractSocket::SocketError err)
{
    if (err == QAbstractSocket::RemoteHostClosedError) {
        //Probably there was no error and the connection was closed because all
        //the data was transferred. Unfortunately the logserver does not send anything
        //at the end of the data transfer.
        if (dataSize_ > 0) {
            versionStatus_ = VersionFetched;
            buildVersion();
            soc_->abort();
            Q_EMIT finished();
            return;
        }
    }

    versionStatus_ = VersionFailedToFetch;
    soc_->abort();
    UiLog().dbg() << "OutputVersionClient::slotError logserver[" + longName() +
                     " ] could not get version! version is set to " << version_;
    Q_EMIT error(soc_->errorString());
}

void OutputVersionClient::slotRead()
{
    const qint64 size = 64*1024;
    char buf[size];
    qint64 len = 0;

    while((len = soc_->read(buf,size)) > 0)
    {
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
        UiLog().dbg() << UI_FN_INFO << "buf=" << buf;
#endif

        std::string err;
        if(dataSize_ + len  < maxDataSize_)
        {
            memcpy(data_+dataSize_,buf,len);
            dataSize_+=len;
            data_[dataSize_] = '\0';
        } else {
            soc_->abort();
            Q_EMIT error("write failed");
        }
    }
}

void OutputVersionClient::getVersion()
{
    UiLog().dbg() << "OutputVersionClient::getVersion";
    versionStatus_ = VersionBeingFetched;
    version_ = 0;
    dataSize_ = 0;
    connectToHost(host_,port_);
}

void OutputVersionClient::buildVersion()
{
    try {
        version_ = std::stoi(data_);
    } catch (...) {
        version_ = 0;
    }

    if (version_ != 2) {
        version_ = 0;
    }
    UiLog().dbg() << UI_FN_INFO << "logserver[" + longName() + "] version=" <<
                          version_;
}
