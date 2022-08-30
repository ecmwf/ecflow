//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputFetchTask.hpp"

#include "UiLog.hpp"

//#define OUTPUTFETCHTASK_DEBUG__

//========================================
//
// OutputFetchTask
//
//========================================

void OutputFetchTask::clear()
{
    queue_ = nullptr;
    status_ = NoStatus;
    runCondition_ = NoCondition;
    server_=nullptr;
    node_=nullptr;
    filePath_.clear();
}

void OutputFetchTask::setQueue(OutputFetchQueue* q)
{
    queue_ = q;
}

bool OutputFetchTask::checRunCondition(OutputFetchTask* prev) const
{
    if (prev && runCondition_ == RunIfPrevFailed && prev->status_ == SucceededStatus) {
        return false;
    }
    return true;
}

void OutputFetchTask::succeed()
{
    status_ = SucceededStatus;
    if (queue_) {
        queue_->taskSucceeded(this);
    }
}

void OutputFetchTask::finish()
{
    status_ = FinishedStatus;
    if (queue_) {
        queue_->taskFinished(this);
    }
}

void OutputFetchTask::fail()
{
    status_ = FailedStatus;
    if (queue_) {
        queue_->taskFailed(this);
    }
}

std::string OutputFetchTask::print() const
{
    return name_ + "[status=" + std::to_string(status_) + ",runCondition=" +  std::to_string(runCondition_) +
            ",filePath=" + filePath_ + "]";
}


//========================================
//
// OutputFetchQueue
//
//========================================

void OutputFetchQueue::clear()
{
    status_ = IdleState;
    for (auto t: queue_) {
        t->stop();
        t->setQueue(nullptr);
    }
    queue_.clear();
}

void OutputFetchQueue::add(OutputFetchTask* t)
{
    t->setQueue(this);
    queue_.push_back(t);
}

void OutputFetchQueue::run()
{
    if (!queue_.empty()) {
        status_ = RunningState;
        queue_.front()->run();
    } else {
        finish();
    }
}

void OutputFetchQueue::next()
{
#ifdef OUTPUTFETCHTASK_DEBUG__
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
#ifdef OUTPUTFETCHTASK_DEBUG__
                    UiLog().dbg() << " skip current";
#endif
                    next();
                } else {
#ifdef OUTPUTFETCHTASK_DEBUG__
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

void OutputFetchQueue::finish(OutputFetchTask* lastTask)
{
#ifdef OUTPUTFETCHTASK_DEBUG__
    UI_FN_DBG
#endif
    clear();
    if (lastTask && lastTask->node()) {
        owner_->fetchQueueFinished(lastTask->filePath(), lastTask->node());
    }
}

void OutputFetchQueue::taskSucceeded(OutputFetchTask* t)
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

void OutputFetchQueue::taskFinished(OutputFetchTask*)
{
    if (status_ == RunningState) {
        next();
    } else {
        clear();
    }
}

void OutputFetchQueue::taskFailed(OutputFetchTask*)
{
#ifdef OUTPUTFETCHTASK_DEBUG__
    UI_FN_DBG
#endif
    if (status_ == RunningState) {
        next();
    } else {
        clear();
    }
}

std::string OutputFetchQueue::print() const
{
    std::string s = "OutpuFetchQueue[";
    for (auto t: queue_) {
        s += t->name() + ",";
    }
    s[s.size()-1] = ']';
    return s;
}


std::ostream&  operator <<(std::ostream &stream,OutputFetchTask* t)
{
    stream << t->print();
    return stream;
}

std::ostream&  operator <<(std::ostream &stream,OutputFetchQueue* q)
{
    stream << q->print();
    return stream;
}
