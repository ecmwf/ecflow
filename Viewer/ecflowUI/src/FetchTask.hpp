/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_FetchTask_HPP
#define ecflow_viewer_FetchTask_HPP

#include <deque>
#include <map>
#include <sstream>
#include <string>

#include "VDir.hpp"
#include "VFile.hpp"

class AbstractFetchTask;
class FetchQueue;
class FetchQueueOwner;
class ServerHandler;
class VNode;
class VReply;

class FetchTaskFactory {
public:
    explicit FetchTaskFactory(const std::string&);
    virtual ~FetchTaskFactory() = default;

    virtual AbstractFetchTask* make(FetchQueueOwner* owner) = 0;
    static AbstractFetchTask* create(const std::string& name, FetchQueueOwner* owner);

private:
    explicit FetchTaskFactory(const FetchTaskFactory&)   = delete;
    FetchTaskFactory& operator=(const FetchTaskFactory&) = delete;
};

template <class T>
class FetchTaskMaker : public FetchTaskFactory {
    AbstractFetchTask* make(FetchQueueOwner* owner) override { return new T(owner); }

public:
    explicit FetchTaskMaker(const std::string& name) : FetchTaskFactory(name) {}
};

class AbstractFetchTask {
    friend class FetchQueue;

public:
    enum Status { NoStatus, SucceededStatus, FinishedStatus, FailedStatus, RunningStatus };
    enum RunCondition { NoCondition, RunIfPrevFailed };
    AbstractFetchTask(const std::string& name, FetchQueueOwner* owner) : name_(name), owner_(owner) {}
    virtual ~AbstractFetchTask() = default;
    virtual void run()           = 0;
    virtual void stop() {}
    virtual void clear();
    const std::string& name() const { return name_; }
    VNode* node() const { return node_; }
    const std::string filePath() const { return filePath_; }
    void setRunCondition(RunCondition c) { runCondition_ = c; }
    bool checRunCondition(AbstractFetchTask* prev) const;
    void setAppendResult(bool b) { appendResult_ = b; }
    void reset(const std::string& filePath) { filePath_ = filePath; }
    void reset(ServerHandler* server, VNode* node, const std::string& filePath);
    void reset(ServerHandler* server,
               VNode* node,
               const std::string& filePath,
               size_t deltaPos,
               unsigned int modTime,
               const std::string& checkSum,
               bool useCache);
    void setDeltaPos(size_t deltaPos) { deltaPos_ = deltaPos; }
    void setUseCache(bool useCache) { useCache_ = useCache; }
    void setUseMetaData(bool b) { useMetaData_ = b; }
    std::string print() const;

protected:
    void succeed();
    void finish();
    void fail();
    void setQueue(FetchQueue*);

    std::string name_;
    FetchQueueOwner* owner_{nullptr};
    FetchQueue* queue_{nullptr};
    Status status_{NoStatus};
    RunCondition runCondition_{NoCondition};
    std::string filePath_;
    ServerHandler* server_{nullptr};
    VNode* node_{nullptr};
    bool appendResult_{false};
    size_t deltaPos_{0};
    unsigned int modTime_{0};
    std::string checkSum_;
    bool useCache_{false};
    bool useMetaData_{true};
};

// Simple queue to manage the various fetch tasks defined by the owner
// (OutputFileProvider or OutputDirProvider). The queue does not take ownership
// of the tasks and has two modes:
//  - RunUntilFirstSucceed: runs until a task manages to fetch the
//            logfile and calls fetchQueueSucceeded() on the owner.
//  - RunAll: runs all the tasks
// If all the tasks were run fetchQueueFinished() is called on the owner.

class FetchQueue {
public:
    enum Policy { RunUntilFirstSucceeded, RunAll };
    FetchQueue(Policy policy, FetchQueueOwner* owner) : policy_(policy), owner_(owner) {}
    void add(AbstractFetchTask* t);
    void run();
    void clear();
    Policy policy() const { return policy_; }
    void setPolicy(Policy p) {
        clear();
        policy_ = p;
    }
    bool isEmpty() const { return queue_.empty(); }
    std::size_t size() const { return queue_.size(); }
    FetchQueueOwner* owner() const { return owner_; }
    std::string print() const;

    void taskSucceeded(AbstractFetchTask*);
    void taskFinished(AbstractFetchTask*);
    void taskFailed(AbstractFetchTask*);

private:
    enum Status { IdleState, RunningState };
    void next();
    void finish(AbstractFetchTask* lastTask = nullptr);

    Policy policy_{RunUntilFirstSucceeded};
    FetchQueueOwner* owner_{nullptr};
    std::deque<AbstractFetchTask*> queue_;
    Status status_{IdleState};
};

class FetchQueueOwner {
public:
    FetchQueueOwner() = default;
    virtual ~FetchQueueOwner();
    virtual void clear();
    virtual VReply* theReply() const = 0;
    virtual VFile_ptr findInCache(const std::string& /*fileName*/) { return nullptr; }
    virtual void addToCache(VFile_ptr) {}
    virtual void fetchQueueSucceeded()                                               = 0;
    virtual void fetchQueueFinished(const std::string& filePath, VNode* n = nullptr) = 0;
    virtual void progressStart(const std::string& /*msg*/, int /*max*/) {}
    virtual void progressUpdate(const std::string& /*msg*/, int /*value*/) {}
    virtual void progressStop() {}
    virtual VDir_ptr dirToFile(const std::string& /*fileName*/) const { return nullptr; }
    bool isEmpty() const;

protected:
    AbstractFetchTask* makeFetchTask(const std::string& name);

    FetchQueue* fetchQueue_{nullptr};
    std::map<std::string, std::deque<AbstractFetchTask*>> fetchTasks_;
    std::map<std::string, std::deque<AbstractFetchTask*>> unusedTasks_;
};

std::ostream& operator<<(std::ostream&, AbstractFetchTask*);
std::ostream& operator<<(std::ostream&, FetchQueue*);

#endif /* ecflow_viewer_FetchTask_HPP */
