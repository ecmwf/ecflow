//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTFETCHTASK_HPP
#define OUTPUTFETCHTASK_HPP

#include <deque>
#include <string>

class OutputFetchQueue;
class ServerHandler;
class VNode;

class OutputFetchTask
{
    friend class OutputFetchQueue;
public:
    enum Status {NoStatus, SucceededStatus, FinishedStatus, FailedStatus, RunningStatus};
    enum RunCondition {NoCondition, RunIfPrevFailed};
    OutputFetchTask() = default;
    virtual ~OutputFetchTask() = default;
    virtual void run()=0;
    virtual void stop() {}
    virtual void clear();
    VNode* node() const {return node_;}
    void setRunCondition(RunCondition c) {runCondition_ = c;}
    bool checRunCondition(OutputFetchTask* prev) const;

protected:
    void succeed();
    void finish();
    void fail();
    void setQueue(OutputFetchQueue*);

    OutputFetchQueue* queue_{nullptr};
    Status status_{NoStatus};
    RunCondition runCondition_{NoCondition};
    std::string filePath_;
    ServerHandler* server_{nullptr};
    VNode* node_{nullptr};
};

class OutputFetchQueueOwner
{
public:
    OutputFetchQueueOwner() = default;
    virtual void fetchQueueSucceeded() = 0;
    virtual void fetchQueueFinished(VNode* n= nullptr) = 0;
};


// The queue runs until a task manages to fetch the
// logfile and calls taskSucceeded. The queue does not take ownership
// of the tasks.

class OutputFetchQueue
{
public:
    enum Policy {RunUntilFirstSucceed, RunAll};
    OutputFetchQueue(Policy policy, OutputFetchQueueOwner *owner) : policy_(policy), owner_(owner) {}
    void add(OutputFetchTask* t);
    void run();
    void clear();
    bool isEmpty() const {return queue_.empty();}

    void taskSucceeded(OutputFetchTask*);
    void taskFinished(OutputFetchTask*);
    void taskFailed(OutputFetchTask*);

protected:
    enum Status {IdleState, RunningState};
    void next();
    void finish(OutputFetchTask* lastTask=nullptr);

    Policy policy_{RunUntilFirstSucceed};
    OutputFetchQueueOwner *owner_{nullptr};
    std::deque<OutputFetchTask*> queue_;
    Status status_{IdleState};
};

#endif // OUTPUTFETCHTASK_HPP


