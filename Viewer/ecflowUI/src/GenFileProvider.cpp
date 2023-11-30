/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "GenFileProvider.hpp"

#include "UiLog.hpp"
#include "VConfig.hpp"
#include "VReply.hpp"

#define UI_FILEPROVIDER_TASK_DEBUG__

GenFileProvider::GenFileProvider(GenFileReceiver* provider) : reply_(new VReply()), provider_(provider) {
    fetchQueue_ = new FetchQueue(FetchQueue::RunAll, this);
}

GenFileProvider::~GenFileProvider() {
    if (reply_) {
        delete reply_;
    }
}

void GenFileProvider::clear() {
    reply_->reset();
    filesToFetch_.clear();
}

void GenFileProvider::fetchFiles(const std::vector<std::string>& fPaths) {
    reply_->reset();

    filesToFetch_ = fPaths;

    // Update the fetch tasks and process them.
    fetchQueue_->clear();
    fetchQueue_->setPolicy(FetchQueue::RunAll);
    Q_ASSERT(fetchQueue_->isEmpty());

    // we assume we have one task per path
    for (auto p : filesToFetch_) {
        AbstractFetchTask* t = nullptr;
        if (VConfig::instance()->proxychainsUsed()) {
            t = makeFetchTask("file_transfer");
        }
        else {
            t = makeFetchTask("file_local");
        }
        Q_ASSERT(t);
        t->reset(p);
        t->setAppendResult(true);
        t->setUseMetaData(false);
        fetchQueue_->add(t);
    }

    Q_ASSERT(fetchQueue_->size() == filesToFetch_.size());

    // #ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "queue=" << fetchQueue_;
    // #endif
    fetchQueue_->run();
}

void GenFileProvider::fetchFile(const std::string& fPath) {
    filesToFetch_ = {fPath};

    // Update the fetch tasks and process them. The queue runs until any task can fetch
    // the logfile
    fetchQueue_->clear();
    fetchQueue_->setPolicy(FetchQueue::RunUntilFirstSucceeded);
    Q_ASSERT(fetchQueue_->isEmpty());
    AbstractFetchTask* t = nullptr;
    //    UiLog().dbg() << UI_FN_INFO << "proxychains=" << VConfig::instance()->proxychainsUsed();
    if (VConfig::instance()->proxychainsUsed()) {
        t = makeFetchTask("file_transfer");
    }
    else {
        t = makeFetchTask("file_local");
    }
    Q_ASSERT(t);
    t->setRunCondition(AbstractFetchTask::NoCondition);
    t->reset(fPath);
    t->setAppendResult(true);
    t->setUseMetaData(false);
    fetchQueue_->add(t);

#ifdef UI_OUTPUTFILEPROVIDER_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "queue=" << fetchQueue_;
#endif
    fetchQueue_->run();
}

void GenFileProvider::fetchQueueSucceeded() {
    provider_->fileFetchFinished(reply_);
    reply_->reset();
    filesToFetch_.clear();
}

void GenFileProvider::fetchQueueFinished(const std::string& /*filePath*/, VNode*) {
    if (fetchQueue_->policy() == FetchQueue::RunAll && !reply_->tmpFiles().empty()) {
        fetchQueueSucceeded();
    }
    else {
        if (reply_->errorText().empty()) {
            reply_->setErrorText("Failed to fetch file(s)!");
        }
        provider_->fileFetchFailed(reply_);
        reply_->reset();
        filesToFetch_.clear();
    }
}

GenFileReceiver::GenFileReceiver() {
    fetchManager_ = new GenFileProvider(this);
}

GenFileReceiver::~GenFileReceiver() {
    delete fetchManager_;
}
