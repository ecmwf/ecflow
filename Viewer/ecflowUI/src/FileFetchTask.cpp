//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "FileFetchTask.hpp"

#include <cassert>

#include "OutputFileClient.hpp"
#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VFileTransfer.hpp"
#include "VNode.hpp"
#include "VReply.hpp"

#define UI_FILEPROVIDER_TASK_DEBUG__

//=================================
//
// FileFetchLocalTask
//
//=================================

FileFetchLocalTask::FileFetchLocalTask(FetchQueueOwner* owner) :
     AbstractFetchTask("FileFetchLocal", owner) {}

// try to read the logfile from the disk (if the settings allow it)
void FileFetchLocalTask::run()
{
#ifdef UI_FILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif
    auto reply = owner_->theReply();
    //we do not want to delete the file once the VFile object is destroyed!!
    VFile_ptr f(VFile::create(filePath_,false));
    if(f->exists())
    {
        reply->fileReadMode(VReply::LocalReadMode);
        reply->addLogTryEntry("read file from disk: OK");

        f->setSourcePath(f->path());
        f->setFetchMode(VFile::LocalFetchMode);
        f->setFetchDate(QDateTime::currentDateTime());
        if (appendResult_) {
            reply->appendTmpFile(f);
        } else {
            reply->tmpFile(f);
        }
        succeed();
        return ;
    }
    reply->addLogTryEntry("read file from disk: NO ACCESS");
    reply->appendErrorText("Failed to read file from disk\n");
    finish();
}

//=================================
//
// FileFetchTransferTask
//
//=================================

FileFetchTransferTask::FileFetchTransferTask(FetchQueueOwner* owner) :
     QObject(nullptr), AbstractFetchTask("FileFetchTransfer", owner)
{
}

void FileFetchTransferTask::stopTransfer()
{
    if (transfer_) {
        transfer_->stopTransfer(false);
     }
}

void FileFetchTransferTask::stop()
{
    stopTransfer();
    AbstractFetchTask::clear();
}

void FileFetchTransferTask::clear()
{
    stopTransfer();
    AbstractFetchTask::clear();
}

//Fetch the file asynchronously via ssh. The output client will call clientFinished() or
//clientError eventually!!
void FileFetchTransferTask::run()
{
#ifdef UI_FILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_ << " deltaPos=" << deltaPos_;
#endif

    UiLog().dbg() << UI_FN_INFO << "proxychains=" << VConfig::instance()->proxychainsUsed();
    assert(VConfig::instance()->proxychainsUsed());

    assert(queue_);
    auto reply = queue_->owner()->theReply();
    assert(reply);

    auto socksPort = VFileTransfer::socksPort();
    if (socksPort.isEmpty()) {
        reply->addLogTryEntry("fetch file from SOCKS host: NOT DEFINED");
        finish();
        return;
    }

    if (!transfer_) {
        transfer_ = new VFileTransfer(this);

        connect(transfer_, SIGNAL(transferFinished()),
                this, SLOT(transferFinished()));

        connect(transfer_, SIGNAL(transferFailed(QString)),
                this, SLOT(transferFailed(QString)));
    }

    Q_ASSERT(transfer_);
    if (useMetaData_) {
        if (deltaPos_ > 0) {
            transfer_->transferLocalViaSocks(QString::fromStdString(filePath_),
                           VFileTransfer::BytesFromPos, deltaPos_,  modTime_, checkSum_);
        } else {
            transfer_->transferLocalViaSocks(QString::fromStdString(filePath_),
                           VFileTransfer::AllBytes, 0,  modTime_, checkSum_);
        }
    } else {
        if (deltaPos_ > 0) {
            transfer_->transferLocalViaSocks(QString::fromStdString(filePath_),
                           VFileTransfer::BytesFromPos, deltaPos_);
        } else {
            transfer_->transferLocalViaSocks(QString::fromStdString(filePath_),
                           VFileTransfer::AllBytes, 0);
        }
    }
    owner_->progressStart("Getting local file <i>" + filePath_ +  "</i> from SOCKS host via scp", 0);
}

void FileFetchTransferTask::transferFinished()
{
    auto reply = owner_->theReply();
    reply->setInfoText("");
    reply->fileReadMode(VReply::TransferReadMode);

    if (deltaPos_ > 0) {
        reply->addLogTryEntry("fetch file increment from SOCKS host via scp : OK");
    } else {
        reply->addLogTryEntry("fetch file from SOCKS host via scp : OK");
    }

    assert(transfer_);
    auto tmp = transfer_->result();
    assert(tmp);
    if (tmp) {
#ifdef UI_FILEPROVIDER_TASK_DEBUG__
        UiLog().dbg() << UI_FN_INFO << "tmp size=" << tmp->sizeInBytes();
#endif

        tmp->setLog(reply->log());

        //Files retrieved from the log server are automatically added to the cache!
        //To make it work sourcePath must be set on resFile_ !!!
        if (useCache_ && deltaPos_ == 0) {
            owner_->addToCache(tmp);
        }

        if (appendResult_) {
            reply->appendTmpFile(tmp);
        } else {
            reply->tmpFile(tmp);
        }
        transfer_->clear();
        owner_->progressStop();
        succeed();
    } else {
        transferFailed("No contents was transferred");
    }
}

void FileFetchTransferTask::transferProgress(QString /*msg*/,int /*value*/)
{
    //owner_->owner_->infoProgressUpdate(msg.toStdString(),value);
}

void FileFetchTransferTask::transferFailed(QString msg)
{
#ifdef UI_FILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "msg=" << msg;
#endif
    assert(transfer_);
    auto reply = owner_->theReply();
    reply->addLogTryEntry("fetch file from SOCKS host via scp : FAILED");
    reply->appendErrorText("Failed to fetch from SOCKS host via scp\n" + msg.toStdString());
    owner_->progressStop();
    transfer_->clear();
    fail();
}

//=================================
//
// FileFetchCacheTask
//
//=================================

FileFetchCacheTask::FileFetchCacheTask(FetchQueueOwner* owner) :
    AbstractFetchTask("FileFetchCache", owner) {}

// Try to fetch the logfile from the local cache
void FileFetchCacheTask::run()
{
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_ << " useCache=" << useCache_;
#endif

    assert(node_);

    //We try use the cache
    if(useCache_)
    {
        //Check if the given output is already in the cache
        VFile_ptr f=owner_->findInCache(filePath_);
        if (f)
        {
#ifdef UI_FILEPROVIDER_TASK_DEBUG__
            UiLog().dbg() << " File found in cache";
#endif
            f->setCached(true);
            f->setTransferDuration(0);
            auto reply = owner_->theReply();
            reply->setInfoText("");
            reply->fileReadMode(VReply::LogServerReadMode);
            reply->setLog(f->log());
            reply->addLogRemarkEntry("File were read from cache.");
            if (appendResult_) {
                reply->appendTmpFile(f);
            } else {
                reply->tmpFile(f);
            }
            succeed();
            return;
        }
    }
    finish();
}

//=================================
//
// FileFetchLogServerTask
//
//=================================

FileFetchLogServerTask::FileFetchLogServerTask(FetchQueueOwner* owner) :
     QObject(nullptr), AbstractFetchTask("FileFetchLogServer", owner)
{
}

FileFetchLogServerTask::~FileFetchLogServerTask()
{
    if(client_) {
        client_->disconnect(this);
    }
}

void FileFetchLogServerTask::deleteClient()
{
    if(client_) {
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
        UI_FN_DBG
#endif
        client_->disconnect(this);
        client_->deleteLater();
        client_ = nullptr;
    }
}

void FileFetchLogServerTask::stop()
{
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
    UI_FN_DBG
#endif
    AbstractFetchTask::clear();
    if (status_ == RunningStatus) {
        deleteClient();
    }
}

void FileFetchLogServerTask::clear()
{
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
    UI_FN_DBG
#endif
      stop();
}

//Create an output client (to access the logserver) and ask it to the fetch the
//file asynchronously. The output client will call clientFinished() or
//clientError() eventually!!
void FileFetchLogServerTask::run()
{
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "filePath=" << filePath_;
#endif
    std::string host, port;
    assert(node_);

    // First try the user defined logserver, then the system defined one
    bool userLogServerUsed = node_->userLogServer(host,port);
    bool sysLogServerUsed = false;
    if (!userLogServerUsed) {
        sysLogServerUsed = node_->logServer(host,port);
    }
    Q_ASSERT(!userLogServerUsed || !sysLogServerUsed);

    if (userLogServerUsed || sysLogServerUsed) {
        Q_ASSERT(userLogServerUsed || sysLogServerUsed);
        if (client_ && (client_->host() != host || client_->portStr() != port)) {
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
        UiLog().dbg() << " host/port does not match! Create new client";
#endif
        deleteClient();
        }

        if (!client_) {
            client_=new OutputFileClient(host,port,this);

            connect(client_,SIGNAL(error(QString)),
                    this,SLOT(clientError(QString)));

            connect(client_,SIGNAL(progress(QString,int)),
                    this,SLOT(clientProgress(QString,int)));

            connect(client_,SIGNAL(finished()),
                    this,SLOT(clientFinished()));
        }

        Q_ASSERT(client_);
#ifdef  UI_FILEPROVIDER_TASK_DEBUG__
        UiLog().dbg() << " use logserver=" << client_->longName();
#endif

        owner_->progressStart("Getting file <i>" + filePath_ +  "</i> from" +
                              ((userLogServerUsed)?"<b>user defined</b>":"") +
                              " log server <i> " + client_->longName() + "</i>",0);

        VDir_ptr dir=owner_->dirToFile(filePath_);
        client_->setDir(dir);

        // fetch the file asynchronously
        client_->getFile(filePath_, deltaPos_, modTime_, checkSum_);
        return;
    }

    //If we are here there is no output client defined/available
    deleteClient();

    owner_->theReply()->addLogTryEntry("fetch file from logserver: NOT DEFINED");
    finish();
}

void FileFetchLogServerTask::clientFinished()
{
    VFile_ptr tmp = client_->result();
    //assert(tmp);
    assert(client_);

    if (tmp) {
        client_->clearResult();

        //Files retrieved from the log server are automatically added to the cache!
        //sourcePath must be already set on tmp
        if (useCache_ && !tmp->hasDeltaContents()) {
            owner_->addToCache(tmp);
        }

        auto reply = owner_->theReply();
        reply->setInfoText("");
        reply->fileReadMode(VReply::LogServerReadMode);

        if (tmp->hasDeltaContents()) {
            reply->addLogTryEntry("fetch file increment from logserver=" + client_->longName() + ": OK");
        } else {
            reply->addLogTryEntry("fetch file from logserver=" + client_->longName() + ": OK");
        }

        tmp->setLog(reply->log());

        if (appendResult_) {
            reply->appendTmpFile(tmp);
        } else {
            reply->tmpFile(tmp);
        }

        client_->clearResult();
        owner_->progressStop();
        succeed();
    } else {
        clientError("No contents were transferred");
    }
}

void FileFetchLogServerTask::clientProgress(QString msg,int value)
{
    owner_->progressUpdate(msg.toStdString(),value);
}

void FileFetchLogServerTask::clientError(QString msg)
{
    assert(client_);
    owner_->progressStop();
    auto reply = owner_->theReply();
    reply->addLogTryEntry("fetch file from logserver=" + client_->longName() + ": FAILED");
    reply->appendErrorText("Failed to fetch file from logserver=" + client_->longName() + "\n" + msg.toStdString());
    client_->clearResult();
    fail();
}


static FetchTaskMaker<FileFetchLocalTask> maker1("file_local");
static FetchTaskMaker<FileFetchTransferTask> maker2("file_transfer");
static FetchTaskMaker<FileFetchCacheTask> maker3("file_cache");
static FetchTaskMaker<FileFetchLogServerTask> maker4("file_logserver");

