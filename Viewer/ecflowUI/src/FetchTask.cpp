//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "FetchTask.hpp"

#include <cassert>
#include <map>

#include "UiLog.hpp"
#include "UIDebug.hpp"

//#define UI_FETCHTASK_DEBUG__

static std::map<std::string,FetchTaskFactory*>* makers = nullptr;

FetchTaskFactory::FetchTaskFactory(const std::string& name)
{
    if(makers == nullptr)
        makers = new std::map<std::string,FetchTaskFactory*>;

    // Put in reverse order...
    (*makers)[name] = this;
}

FetchTaskFactory::~FetchTaskFactory()
{
    // Not called
}

AbstractFetchTask* FetchTaskFactory::create(const std::string& name, FetchQueueOwner* owner)
{
    AbstractFetchTask* t=nullptr;
    auto j = makers->find(name);
    if(j != makers->end()) {
        t = (*j).second->make(owner);
    }

    UI_ASSERT(t, UI_FN_INFO + " unsupported name=" + name);
    return t;
}

//========================================
//
// OutputFetchTask
//
//========================================

void AbstractFetchTask::clear()
{
    queue_ = nullptr;
    status_ = NoStatus;
    runCondition_ = NoCondition;
    server_=nullptr;
    node_=nullptr;
    filePath_.clear();
    deltaPos_=0;
    useCache_=false;
}

void AbstractFetchTask::reset(ServerHandler* server,VNode* node,const std::string& filePath)
{
    server_=server;
    node_=node;
    filePath_=filePath;
}

void AbstractFetchTask::reset(ServerHandler* server,VNode* node,const std::string& filePath,
           size_t deltaPos, unsigned int modTime, const std::string& checkSum, bool useCache)
{
    server_=server;
    node_=node;
    filePath_=filePath;
    deltaPos_=deltaPos;
    modTime_=modTime;
    checkSum_=checkSum;
    useCache_=useCache;
}

void AbstractFetchTask::setQueue(FetchQueue* q)
{
    queue_ = q;
}

bool AbstractFetchTask::checRunCondition(AbstractFetchTask* prev) const
{
    if (prev && runCondition_ == RunIfPrevFailed && prev->status_ == SucceededStatus) {
        return false;
    }
    return true;
}

void AbstractFetchTask::succeed()
{
    status_ = SucceededStatus;
    if (queue_) {
        queue_->taskSucceeded(this);
    }
}

void AbstractFetchTask::finish()
{
    status_ = FinishedStatus;
    if (queue_) {
        queue_->taskFinished(this);
    }
}

void AbstractFetchTask::fail()
{
    status_ = FailedStatus;
    if (queue_) {
        queue_->taskFailed(this);
    }
}

std::string AbstractFetchTask::print() const
{
    return name_ + "[status=" + std::to_string(status_) + ",runCondition=" +  std::to_string(runCondition_) +
            ",filePath=" + filePath_ + "]";
}


//========================================
//
// OutputFetchQueue
//
//========================================

void FetchQueue::clear()
{
    status_ = IdleState;
    for (auto t: queue_) {
        t->stop();
        t->setQueue(nullptr);
    }
    queue_.clear();
}

void FetchQueue::add(AbstractFetchTask* t)
{
    assert(t);
    t->setQueue(this);
    queue_.push_back(t);
}

void FetchQueue::run()
{
    if (!queue_.empty()) {
        status_ = RunningState;
        queue_.front()->run();
    } else {
        finish();
    }
}

void FetchQueue::next()
{
#ifdef UI_FETCHTASK_DEBUG__
    UiLog().dbg() << "OutputFetchQueue::next";
#endif
    if (status_ == RunningState) {
        if (!queue_.empty()) {
            auto prev = queue_.front();
            queue_.pop_front();
            prev->setQueue(nullptr);
            if (!queue_.empty()) {
                auto current = queue_.front();
                if (!current->checRunCondition(prev)) {
#ifdef UI_FETCHTASK_DEBUG__
                    UiLog().dbg() << " skip current";
#endif
                    next();
                } else {
#ifdef UI_FETCHTASK_DEBUG__
                    UiLog().dbg() << " run current";
#endif
                    current->run();
                }
            } else {
                finish(prev);
            }
        } else {
            finish();
        }
    }
}

void FetchQueue::finish(AbstractFetchTask* lastTask)
{
#ifdef UI_FETCHTASK_DEBUG__
    UI_FN_DBG
#endif
    clear();
    if (lastTask && lastTask->node()) {
        owner_->fetchQueueFinished(lastTask->filePath(), lastTask->node());
    }
}

void FetchQueue::taskSucceeded(AbstractFetchTask* t)
{
    if (status_ == RunningState) {
        if (policy_ == RunUntilFirstSucceeded) {
            clear();
            owner_->fetchQueueSucceeded();
        } else {
            next();
        }
    }
}

void FetchQueue::taskFinished(AbstractFetchTask*)
{
    if (status_ == RunningState) {
        next();
    } else {
        clear();
    }
}

void FetchQueue::taskFailed(AbstractFetchTask*)
{
#ifdef UI_FETCHTASK_DEBUG__
    UI_FN_DBG
#endif
    if (status_ == RunningState) {
        next();
    } else {
        clear();
    }
}

std::string FetchQueue::print() const
{
    std::string s = "OutpuFetchQueue[";
    for (auto t: queue_) {
        s += t->name() + ",";
    }
    s[s.size()-1] = ']';
    return s;
}

//========================================
//
// OutputFetchQueueOwner
//
//========================================

FetchQueueOwner::~FetchQueueOwner()
{
    if (fetchQueue_) {
        delete fetchQueue_;
        fetchQueue_ = nullptr;
    }

    for (auto it: fetchTasks_) {
        for(auto itV: it.second) {
            delete itV;
        }
    }
    fetchTasks_.clear();
    unusedTasks_.clear();
}

void FetchQueueOwner::clear()
{
    // clear the queue and the fetch tasks
    if (fetchQueue_) {
        fetchQueue_->clear();
        for (auto it: fetchTasks_) {
            for(auto itV: it.second) {
                itV->clear();
            }
        }
    }
    unusedTasks_ = fetchTasks_;
}

AbstractFetchTask* FetchQueueOwner::makeFetchTask(const std::string& name)
{
#ifdef UI_FETCHTASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "unusedtasks";
    for(auto t: unusedTasks_) {
        UiLog().dbg() << " " << t.first << " len=" << t.second.size();
    }
    UiLog().dbg() << UI_FN_INFO << "fetchTasks";
    for(auto t: fetchTasks_) {
        UiLog().dbg() << " " << t.first << " len=" << t.second.size();
    }
#endif
    auto it = unusedTasks_.find(name);
    if (it != unusedTasks_.end()) {
        if (!it->second.empty()) {
            auto t = it->second.front();
            it->second.pop_front();
            return t;
        }
    }
#ifdef UI_FETCHTASK_DEBUG__
    UiLog().dbg() << UI_FN_INFO << "create object for name=" << name;
#endif
    AbstractFetchTask *t = FetchTaskFactory::create(name, this);
    fetchTasks_[name].emplace_back(t);
    return t;
}


std::ostream&  operator <<(std::ostream &stream,AbstractFetchTask* t)
{
    stream << t->print();
    return stream;
}

std::ostream&  operator <<(std::ostream &stream,FetchQueue* q)
{
    stream << q->print();
    return stream;
}
