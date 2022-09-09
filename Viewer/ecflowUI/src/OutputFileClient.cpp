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

#include <cstring>

#include "UIDebug.hpp"
#include "UiLog.hpp"

//#define UI_OUTPUTFILECLIENT_DEBUG__
//#define UI_OUTPUTFILECLIENT_DETAILED_DEBUG__

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
    readStarted_ = false;
    reqType_ = NoRequest;
    std::string s;
    if (deltaPos_ > 0) {
        reqType_ = DeltaRequest;
        soc_->write("delta ",6);
        s = std::to_string(deltaPos_) + " " + std::to_string(remoteModTime_) + " " +
               ((remoteCheckSum_.empty())?"x":remoteCheckSum_);
    } else  {
        if (versionClient_->version() == 2) {
            reqType_ = GetFRequest;
            soc_->write("getf ",5);
        } else {
            reqType_ = GetRequest;
            soc_->write("get ",4);
        }
    }

    assert(reqType_ != NoRequest);
    s += " " + remoteFile_ + "\n";
    soc_->write(s.c_str(), strlen(s.c_str())+1);
}

void OutputFileClient::slotError(QAbstractSocket::SocketError err)
{
#ifdef UI_OUTPUTFILECLIENT_DEBUG__
    UiLog().dbg() <<  UI_FN_INFO  << "err=" << soc_->errorString();
#endif

    if (err == QAbstractSocket::RemoteHostClosedError) {
        //Probably there was no error and the connection was closed because all
        //the data was transferred. Unfortunately the logserver does not send anything
        //at the end of the data transfer.
        if(total_ > 0 || reqType_ == DeltaRequest) {
            soc_->abort();
            if(out_)
			{
				out_->setTransferDuration(stopper_.elapsed());
				out_->setFetchDate(QDateTime::currentDateTime());               
                //out_->print();
                out_->close();
			}

			Q_EMIT finished();
            return;
		}
    }

    // an error happened
    soc_->abort();
    if(out_)
    {
        out_->close();
        out_.reset();
    }
    Q_EMIT error(soc_->errorString());;
}

void OutputFileClient::getFile(const std::string& name)
{
    getFile(name, 0, 0, "");
}

void OutputFileClient::getFile(const std::string& name, size_t deltaPos, unsigned int modTime, const std::string& checkSum)
{
    // stop running the current transfer
    soc_->abort();
    clearResult();

#ifdef UI_OUTPUTFILECLIENT_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "name=" << name << " deltaPos=" << deltaPos << " modTime=" << modTime << " checksum=" << checkSum;
#endif

    remoteFile_=name;
    deltaPos_ = deltaPos;
    remoteModTime_ = modTime;
    remoteCheckSum_ = checkSum;

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
    out_->setFetchMode(VFile::LogServerFetchMode);
    out_->setFetchModeStr("served by " + longName());

    total_=0;
    lastProgress_=0;
    readStarted_=false;
    reqType_ = NoRequest;
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
        // parse+remove "header" in "getf" and "delta" modes from the beginnig of the resulting data
        if (!readStarted_ && (reqType_ == GetFRequest || reqType_ == DeltaRequest)) {
            readStarted_ = true;
            // an error code was sent back
            if (!parseResultHeader(buf, len)) {
                soc_->abort();
                clearResult();
#ifdef UI_OUTPUTFILECLIENT_DEBUG__
                UiLog().dbg() << UI_FN_INFO << "error in parsing reult";
#endif
                Q_EMIT error("transfer failed");
                return;
            }
        }

		std::string err;
		if(!out_->write(buf,len,err))
		{
			soc_->abort();
			Q_EMIT error("write failed");
            return;
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

bool OutputFileClient::parseResultHeader(char* buf, quint64& len)
{
    //  format:
    //      retCode:modTime:checkSum:result_text
    //
    //      where:
    //          - retCode is a 1-digit number
    //          - modeTime/checkSum can be empty
    //          - result_text can be empty
    //      note: in delta mode a single "0" message is allowed
    //
    //  e.g.:
    //      0:1662369142:8fssaf:abcd
    //      0:1662369142::abcd
    //      1:::
    //      0

    if ((reqType_ == GetFRequest || reqType_ == DeltaRequest)) {
        if (len <=0) {
            return false;
        }

        // is the whole message is "0" there is no delta to add!!
        if (reqType_ == DeltaRequest && len == 1 && buf[0] == '0') {
            len = 0;
            out_->setSourceModTime(remoteModTime_);
            out_->setSourceCheckSum(remoteCheckSum_);
            return true;
        }
#ifdef UI_OUTPUTFILECLIENT_DETAILD_DEBUG__
        UiLog().dbg() << UI_FN_INFO << "len=" << len << " buf=" << buf ;
#endif
        int pos1=0;
        int pos2=0;

        // get return code
        std::string v;
        if (!getHeaderValue(buf, len, pos1, pos2, v)) {
            return false;
        }
        if (reqType_ == GetFRequest) {
            if (v != "0") {
                return false;
            }
        }
        if (reqType_ == DeltaRequest) {
            // the whole file was sent back
            if (v == "1") {
                deltaPos_ = 0;
                out_->setDeltaContents(false);
            } else if (v != "0") {
                return false;
            }
        }

        // get modtime
        if (pos1 >= pos2 || buf[pos2] != ':') {
            return false;
        }

        // get modtime
        pos1=pos2;
        if (!getHeaderValue(buf, len, pos1, pos2, v)) {
            return false;
        }
        if (!v.empty()) {
            unsigned int uv = 0;
            try {
                uv = std::stoul(v);
            } catch (...) {
                uv = 0;
            }
            out_->setSourceModTime(uv);
        }

        // get checksum
        pos1=pos2;
        if (!getHeaderValue(buf, len, pos1, pos2, v)) {
            return false;
        }
        if (!v.empty()) {
            out_->setSourceCheckSum(v);
        }

        // remove "header" from the bufr
        Q_ASSERT(pos2>=3);
        auto startPos = pos2 + 1;
        Q_ASSERT(startPos>=4);
        for (size_t i=startPos; i < len; i++) {
            buf[i-startPos]=buf[i];
        }
        Q_ASSERT(startPos <= len);
        len -= (startPos);
    }
    return true;
}

bool OutputFileClient::getHeaderValue(char* buf, quint64 len, int pos1, int& pos2, std::string& val)
{
#ifdef UI_OUTPUTFILECLIENT_DETAILD_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "pos1=" << pos1 << " pos2=" << pos2;
#endif
    val = std::string();
    if (pos1==0 || buf[pos1] == ':') {
        const int dLenMax=200;
        char d[dLenMax];
        int dLen=0;
        if (pos1 > 0) {
            pos1++;
        }
        pos2=pos1;
        for (quint64 i=pos1; i < len && dLen < dLenMax-1; i++) {
            if (buf[i] == ':') {
                pos2 = i;
#ifdef UI_OUTPUTFILECLIENT_DETAILD_DEBUG__
                UiLog().dbg() << " pos2=" << pos2;
#endif
                int dSize = pos2-pos1;
                if (dSize <=0) {
                    return true;
                }
                d[dLen] = '\0';
                val = std::string(d);
#ifdef UI_OUTPUTFILECLIENT_DETAILD_DEBUG__
                UiLog().dbg() << " -1=" << d[dLen-1] << " d=" << d << " dLen=" << dLen;
                UiLog().dbg() << " val=" << val << " dLen=" << dLen;
#endif
                return true;
            }
            d[dLen] = buf[i];
            dLen++;
        }
    }
    return false;;
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
    if(!dir_ || deltaPos_ > 0)
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
    getFile(remoteFile_, deltaPos_, remoteModTime_, remoteCheckSum_);
}

void OutputFileClient::slotVersionError(QString errorText)
{
#ifdef UI_OUTPUTFILECLIENT_DETAILED_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "errorText=" << errorText;
#endif
    Q_ASSERT(versionClient_->versionStatus() != OutputVersionClient::VersionNotFetched);
    getFile(remoteFile_, deltaPos_,remoteModTime_, remoteCheckSum_);
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
    UiLog().dbg() << UI_FN_INFO << "logserver[" + longName() +
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
