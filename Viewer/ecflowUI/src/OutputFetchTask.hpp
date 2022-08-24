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
#include <sstream>

class OutputFetchQueue;
class ServerHandler;
class VNode;
class VReply;

class OutputFetchTask
{
    friend class OutputFetchQueue;
public:
    enum Status {NoStatus, SucceededStatus, FinishedStatus, FailedStatus, RunningStatus};
    enum RunCondition {NoCondition, RunIfPrevFailed};
    OutputFetchTask(const std::string& name) : name_(name) {}
    virtual ~OutputFetchTask() = default;
    virtual void run()=0;
    virtual void stop() {}
    virtual void clear();
    const std::string& name() const {return name_;}
    VNode* node() const {return node_;}
    const std::string filePath() const {return filePath_;}
    void setRunCondition(RunCondition c) {runCondition_ = c;}
    bool checRunCondition(OutputFetchTask* prev) const;
    std::string print() const;

protected:
    void succeed();
    void finish();
    void fail();
    void setQueue(OutputFetchQueue*);

    std::string name_;
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
    virtual void fetchQueueFinished(const std::string& filePath, VNode* n=nullptr) = 0;
};


// Simple queue to manage the various fetch tasks defined by the owner
// (OutputFileProvider or OutputDirProvider). The queue does not take ownership
// of the tasks and has two modes:
//  - RunUntilFirstSucceed: runs until a task manages to fetch the
//            logfile and calls fetchQueueSucceeded() on the owner.
//  - RunAll: runs all the tasks
// If all the tasks were run fetchQueueFinished() is called on the owner.

class OutputFetchQueue
{
public:
    enum Policy {RunUntilFirstSucceeded, RunAll};
    OutputFetchQueue(Policy policy, OutputFetchQueueOwner *owner) : policy_(policy), owner_(owner) {}
    void add(OutputFetchTask* t);
    void run();
    void clear();
    bool isEmpty() const {return queue_.empty();}
    std::string print() const;

    void taskSucceeded(OutputFetchTask*);
    void taskFinished(OutputFetchTask*);
    void taskFailed(OutputFetchTask*);

protected:
    enum Status {IdleState, RunningState};
    void next();
    void finish(OutputFetchTask* lastTask=nullptr);

    Policy policy_{RunUntilFirstSucceeded};
    OutputFetchQueueOwner *owner_{nullptr};
    std::deque<OutputFetchTask*> queue_;
    Status status_{IdleState};
};

std::ostream&  operator <<(std::ostream &,OutputFetchTask*);
std::ostream&  operator <<(std::ostream &,OutputFetchQueue*);

#endif // OUTPUTFETCHTASK_HPP


