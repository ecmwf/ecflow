/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ServerComQueue.hpp"

#include <QApplication>

#include "ServerComThread.hpp"
#include "ServerHandler.hpp"
#include "UIDebug.hpp"
#include "UiLogS.hpp"
#include "UserMessage.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Log.hpp"

#define _UI_SERVERCOMQUEUE_DEBUG

// This class manages the tasks to be sent to the ServerComThread, which controls
// the communication with the ClientInvoker. The ClientInvoker is hidden from the
// ServerHandler. The ServerHandler defines a task and sends it to
// ServerComQueue, which then passes it on to the ClientInvoker. When a task is finished
// ServerComQueue notifies the ServerHandler about it.

ServerComQueue::ServerComQueue(ServerHandler* server, ClientInvoker* client)
    : QObject(server),
      server_(server),
      client_(client),
      comThread_(nullptr),
      timeout_(5),
      ctStartTimeout_(1000),
      ctStartWaitTimeout_(500), // wait() is a blocking call, so it should be short
      startTimeoutTryCnt_(0),
      ctMaxStartTimeoutTryCnt_(4),
      state_(NoState), // the queue is enabled but not running
      taskStarted_(false),
      taskIsBeingFinished_(false),
      taskIsBeingFailed_(false) {
    timer_ = new QTimer(this);
    timer_->setInterval(timeout_);

    connect(timer_, SIGNAL(timeout()), this, SLOT(slotRun()));

    if (client_) {
        createThread();
    }
}

// the deletion is inititated by logout()
ServerComQueue::~ServerComQueue() {
    UI_ASSERT(state_ == DisabledState, "state=" << state_);
    Q_ASSERT(comThread_ == nullptr);
}

void ServerComQueue::createThread() {
    assert(client_);

    if (comThread_) {
        delete comThread_;
    }

    // We create a ServerComThread here. At this point the thread is not doing anything.
    comThread_ = new ServerComThread(server_, client_);

    // When the ServerComThread starts it emits a signal that
    // is connected to the queue.
    connect(comThread_, SIGNAL(started()), this, SLOT(slotTaskStarted()));

    // When the ServerComThread finishes it emits a signal that
    // is connected to the queue.
    connect(comThread_, SIGNAL(finished()), this, SLOT(slotTaskFinished()));

    // When there is an error in ServerComThread it emits the
    // failed() signal that is connected to the queue.
    connect(comThread_, SIGNAL(failed(std::string)), this, SLOT(slotTaskFailed(std::string)));

    // The ServerComThread is observing the actual server and its nodes. When there is a change it
    // emits a signal to notify the ServerHandler about it.
    connect(comThread_,
            SIGNAL(nodeChanged(const Node*, std::vector<ecf::Aspect::Type>)),
            server_,
            SLOT(slotNodeChanged(const Node*, std::vector<ecf::Aspect::Type>)));

    connect(comThread_,
            SIGNAL(defsChanged(std::vector<ecf::Aspect::Type>)),
            server_,
            SLOT(slotDefsChanged(std::vector<ecf::Aspect::Type>)));

    connect(comThread_, SIGNAL(rescanNeed()), server_, SLOT(slotRescanNeed()));
}

void ServerComQueue::enable() {
    state_ = NoState;
    start();
}

void ServerComQueue::disable() {
    if (state_ == DisabledState) {
        return;
    }

    UiLogS(server_).dbg() << "ComQueue::disable -->";

    state_ = DisabledState;

    // Remove all tasks
    tasks_.clear();

    // Stop the timer
    stopTimer();

    if (comThread_) {

        // If the comthread is running we need to wait
        // until it finishes its task.
        comThread_->wait();

        UiLogS(server_).dbg() << " queue is disabled";
    }

    // Clear the current task
    if (current_) {
        current_.reset();
    }

    taskStarted_ = false;
}

// This is a special mode to reload the whole ClientInvoker
bool ServerComQueue::prepareReset() {
    if (state_ == DisabledState || state_ == ResetState || state_ == SuspendedState) {
        return false;
    }

    // Stop the timer
    stopTimer();

    // Remove all tasks
    tasks_.clear();

    taskStarted_ = false;

    state_ = ResetState;

    // If the comthread is running we need to wait
    // until it finishes its task.
    if (comThread_ && comThread_->wait(5000)) {
        // The thread cannot be running
        assert(comThread_->isRunning() == false);
        // assert(taskIsBeingFinished_==false);
        // assert(taskIsBeingFailed_==false);
        // assert(!current_);
        return true;
    }

    return false;
}

// This is a special mode to reload the whole ClientInvoker. Must be called after prepareReset returned true;
void ServerComQueue::reset() {
    assert(state_ == ResetState);

    if (comThread_) {

        // The thread cannot be running
        assert(comThread_->isRunning() == false);

        // We send a Reset command to the thread!! This is the only task that is allowed
        // during the reset!!
        VTask_ptr task = VTask::create(VTask::ResetTask);
        tasks_.push_back(task);

        // TODO: why do we not run it directly
        // We start the timer with a shorter interval
        timer_->start(100);
    }
}

void ServerComQueue::endReset() {
    if (state_ == ResetState) {
        state_ = SuspendedState;
        start();
    }
}

// When the queue is started:
//  -it is ready to accept tasks
//  -its timer is running
void ServerComQueue::start() {
    if (!client_ || !comThread_) {
        return;
    }

    if (state_ != DisabledState && state_ != ResetState) {
        UiLogS(server_).dbg() << "ComQueue::start -->";

        // assert(taskIsBeingFinished_==false);
        // assert(taskIsBeingFailed_==false);
        // assert(!current_);

        startTimeoutTryCnt_ = 0;
        taskStarted_        = false;

        // If the comthread is running we need to wait
        // until it finishes its task.
        comThread_->wait();

        state_ = RunningState;

        UiLogS(server_).dbg() << "  thread finished";

        // Starts the timer
        timer_->start(timeout_);

        UiLogS(server_).dbg() << "  timer started";
        UiLogS(server_).dbg() << "<-- ComQueue::start";
    }
}

// The queue contents remains the same but the timer is stopped. Until start() is
// called nothing will be submitted to the queue.
void ServerComQueue::suspend(bool wait) {
    if (state_ != DisabledState && state_ != ResetState && state_ != SuspendedState) {
        state_ = SuspendedState;
        stopTimer();
        if (comThread_ && wait) {
            comThread_->wait();
        }

        // assert(taskIsBeingFinished_==false);
        // assert(taskIsBeingFailed_==false);
        // assert(!current_);
    }
}

void ServerComQueue::stopTimer() {
    timer_->stop();
    startTimeoutTryCnt_ = 0;
}

bool ServerComQueue::hasTask(VTask::Type t) const {
    for (const auto& task : tasks_) {
        if (task && task->type() == t && task->status() != VTask::CANCELLED && task->status() != VTask::ABORTED) {
            return true;
        }
    }
    return false;
}

bool ServerComQueue::isNextTask(VTask::Type t) const {
    return (!tasks_.empty() && tasks_.back()->type() == t);
}

void ServerComQueue::addTask(VTask_ptr task) {
    if (!task) {
        return;
    }

    if (isNextTask(VTask::ZombieListTask) && tasks_.back()->type() == task->type()) {
        return;
    }

    if (state_ == DisabledState || state_ == ResetState || (task && task->type() == VTask::ResetTask)) {
        return;
    }

    tasks_.push_back(task);
    if (state_ != SuspendedState) {
        if (!timer_->isActive()) {
            // we immediately execute the "current" task
            slotRun();
            // and only start the timer after it
            timer_->start(timeout_);
        }
    }
}

void ServerComQueue::addNewsTask() {
    addNewsTask("", "");
}

void ServerComQueue::addNewsTask(const std::string& key, const std::string& value) {
    if (state_ == DisabledState || state_ == ResetState) {
        return;
    }

    if (isNextTask(VTask::NewsTask)) {
        return;
    }

    VTask_ptr task = VTask::create(VTask::NewsTask);
    if (!key.empty()) {
        task->param(key, value);
    }

    addTask(task);
    server_->refreshScheduled();
}

void ServerComQueue::addSyncTask() {
    if (state_ == DisabledState || state_ == ResetState) {
        return;
    }

    if (isNextTask(VTask::SyncTask)) {
        return;
    }

    VTask_ptr task = VTask::create(VTask::SyncTask);
    addTask(task);
}

void ServerComQueue::addSuiteListTask() {
    if (state_ == DisabledState || state_ == ResetState) {
        return;
    }

    if (isNextTask(VTask::SuiteListTask)) {
        return;
    }

    VTask_ptr task = VTask::create(VTask::SuiteListTask);
    addTask(task);
}

void ServerComQueue::addSuiteAutoRegisterTask() {
    if (state_ == DisabledState || state_ == ResetState) {
        return;
    }

    if (isNextTask(VTask::SuiteAutoRegisterTask)) {
        return;
    }

    VTask_ptr task = VTask::create(VTask::SuiteAutoRegisterTask);
    addTask(task);
}

void ServerComQueue::addServerVersionTask() {
    if (state_ == DisabledState || state_ == ResetState) {
        return;
    }

    if (isNextTask(VTask::ServerVersionTask)) {
        return;
    }

    VTask_ptr task = VTask::create(VTask::ServerVersionTask);
    addTask(task);
}

void ServerComQueue::startCurrentTask() {
    if (comThread_) {
        taskStarted_ = false;
        ctStartTime_.start();
        comThread_->task(current_);
    }
}

void ServerComQueue::slotRun() {
    if (!client_ || !comThread_) {
        return;
    }

    if (state_ == DisabledState || state_ == SuspendedState) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        UI_FUNCTION_LOG_S(server_);
        UiLogS(server_).dbg() << " queue is either disabled or suspended";
#endif
        return;
    }

    if (taskIsBeingFinished_ || taskIsBeingFailed_) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        UI_FUNCTION_LOG_S(server_);
        UiLogS(server_).dbg() << " task is either being finished or failed";
#endif
        return;
    }

    if (tasks_.empty() && !current_) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        UI_FUNCTION_LOG_S(server_);
        UiLogS(server_).dbg() << " there are no tasks! Stop timer!";
#endif
        timer_->stop();
        return;
    }

#if 0
    #ifdef _UI_SERVERCOMQUEUE_DEBUG
    if(tasks_.size() > 0)
    {
        UI_FUNCTION_LOG_S(server_);
        UiLog(server_).dbg() << " number of tasks: "  << tasks_.size();
        for(std::deque<VTask_ptr>::const_iterator it=tasks_.begin(); it != tasks_.end(); it++)
        {
            UiLog(server_).dbg() << "  task: " << (*it)->typeString();
        }
    }
    #endif
#endif

    // If a task was sent to the thread but the queue did not get the
    // notification about the thread's start there is a PROBLEM!
    // Note: when the thread starts it emits a signal to the queue
    // and the queue sets taskStarted_ to true. Since this is a queued communication
    // there can be a time lag between the two events. We set a timeout for it!
    // If we pass the timeout we stop the thread and try to resend the task!
    if (current_ && !taskStarted_ && ctStartTime_.elapsed() > ctStartTimeout_) {
        UI_FUNCTION_LOG_S(server_);
        if (startTimeoutTryCnt_ < ctMaxStartTimeoutTryCnt_) {
            UiLogS(server_).warn() << " ServerCom thread does not seem to have started within the allocated timeout. \
                          startTimeoutTryCnt_="
                                   << startTimeoutTryCnt_;

            startTimeoutTryCnt_++;
        }
        else {
            startTimeoutTryCnt_ = 0;

            // So if we are here:
            //-the thread did not emit a notification about its start
            //-the elapsed time since we sent the task to the thread past the timeout.
            //-since the timeout was passed slotRun() has been was called at least startTimeoutTryCnt_ times

            bool running = comThread_->isRunning();

            // Problem 1:
            //-the thread is running
            if (running) {
                UiLogS(server_).warn()
                    << " It seems that the ServerCom thread started but it is in a bad state. Try to run task again.";
            }

            // Problem 2:
            //-the thread is not running
            else {
                UiLogS(server_).warn() << " It seems that the ServerCom thread could not start. Try to run task again.";
            }

            if (comThread_->wait(ctStartWaitTimeout_) == false) {
                UiLogS(server_).warn() << "  Calling wait() on the ServerCom thread failed.";

                // The thread started to run in the meantime. We check its status again at the next slotRun call.
                if (!running && comThread_->isRunning()) {
                    UiLogS(server_).warn() << "  It seems that in the meantime the thread started to run.\
                                   We will check its state again";
                    return;
                }

                // Otherwise there is nothing to do!!
                // We exit here because when we tried to call terminate() it just hung!!
                UI_ASSERT(0, "Cannot stop ServerCom thread, which is in a bad state");
                exit(1);
#if 0
                comThread_->terminate();
                if(comThread_->wait(taskTimeout_) == false)
                {
                    UiLog(server_).err() << "  Calling wait() after terminate() on the ServerCom thread failed";
                    UiLog(server_).dbg() << "Delete the current ComThread and create a new one.";
                    createThread();
                    UI_ASSERT(0,"Cannot stop ServerCom thread that is in a bad state");

                }
                else
                {
                    UiLog(server_).dbg() << "  Terminating ServerCom thread succeeded.";
                }
#endif
            }

            UiLogS(server_).warn() << "  Calling wait() on the ServerCom thread succeeded.";

            Q_ASSERT(comThread_->isRunning() == false);

            if (current_->status() != VTask::CANCELLED && current_->status() != VTask::ABORTED) {
                startCurrentTask();
                return;
            }
            else {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
                UiLogS(server_).dbg() << "  current_ aborted or cancelled. Reset current_ !";
#endif
                current_.reset();
            }
        }
    }

    if (current_) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        // UiLog(server_).dbg() << " still processing reply from previous task";
#endif
        return;
    }

    if (comThread_->isRunning()) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        // UiLog(server_).dbg() << " thread is active";
#endif
        return;
    }

    // We search for the first non-cancelled/aborted task
    while (!tasks_.empty()) {
        current_ = tasks_.front();
        tasks_.pop_front();
        if (current_->status() != VTask::CANCELLED && current_->status() != VTask::ABORTED) {
            break;
        }
        current_.reset();
    }

    if (!current_) {
        timer_->stop();
        return;
    }

#ifdef _UI_SERVERCOMQUEUE_DEBUG
    UiLogS(server_).dbg() << " run task: " << current_->typeString();
#endif

    // Send it to the thread
    startCurrentTask();
}

// This slot is called when ComThread finishes its task. At this point the
// thread is not running so it is safe to access the ClientInvoker!
void ServerComQueue::slotTaskStarted() {
    taskStarted_ = true;
}

// This slot is called when ComThread finishes its task. At this point the
// thread is not running so it is safe to access the ClientInvoker!
void ServerComQueue::slotTaskFinished() {
    if (!client_ || !comThread_) {
        return;
    }

    taskStarted_         = false;
    taskIsBeingFinished_ = true;
    startTimeoutTryCnt_  = 0;

    UiLogS(server_).dbg() << "ComQueue::slotTaskFinished -->";

    // We need to leave the load mode
    endReset();

    // If the current task is empty there must have been an error that was
    // handled by the sloTaskFailed slot.
    if (current_) {
#ifdef _UI_SERVERCOMQUEUE_DEBUG
        UiLogS(server_).dbg() << " reset current_";
#endif

        VTask_ptr task = current_;
        current_.reset();

        // We notify the server that the task has finished and the results can be accessed.
        server_->clientTaskFinished(task, client_->server_reply());
    }

    taskIsBeingFinished_ = false;
}

// This slot is called when the task failed in the ComThread. Right after this signal is emitted
// the thread will finish and and emits the finished() signal that is connected
// to the slotTaskFinished slot.
void ServerComQueue::slotTaskFailed(std::string msg) {
    if (!client_ || !comThread_) {
        return;
    }

    taskStarted_        = false;
    taskIsBeingFailed_  = true;
    startTimeoutTryCnt_ = 0;

    UiLogS(server_).dbg() << "ComQueue::slotTaskFailed -->";
#ifdef _UI_SERVERCOMQUEUE_DEBUG
    if (current_) {
        UiLogS(server_).dbg() << " current_ exists";
    }
    else {
        UiLogS(server_).dbg() << " current_ is null";
    }
#endif

    // We need to leave the load mode
    endReset();

#ifdef _UI_SERVERCOMQUEUE_DEBUG
    if (current_) {
        UiLogS(server_).dbg() << " current_ exists";
    }
    else {
        UiLogS(server_).dbg() << " current_ is null";
    }
#endif

#ifdef _UI_SERVERCOMQUEUE_DEBUG
    UiLogS(server_).dbg() << " reset current_";
#endif
    assert(current_);
    VTask_ptr task = current_;
    current_.reset();

    // We notify the server that the task has failed
    server_->clientTaskFailed(task, msg, client_->server_reply());

    taskIsBeingFailed_ = false;
}

// Only called from the ServerHandler on delete or recreating the client!!!
bool ServerComQueue::logout() {
    state_ = DisabledState;

    // Stop the timer
    timer_->stop();

    // Empty the tasks
    tasks_.clear();

    // The app is not quitting
    if (qApp->property("inQuit").toString() != "1") {

        if (comThread_) {
            // Disconnects all the signals from the thread
            comThread_->disconnect();

            // If the comthread is running we need to wait
            // until it finishes its task.
            if (comThread_->wait(500)) {
                startLogout();
                return false;
                // if it is still running we check it status regularly and
                // start the logout when the thread finishes.
            }
            else {
                UiLogS(server_).warn() << "Waiting for ComThread to stop and start logout!";
                QTimer::singleShot(5000, this, SLOT(slotLogoutCheck()));
            }
        }
        else {
            if (client_) {
                delete client_;
                client_ = nullptr;
            }
            deleteLater();
            return true;
        }
        // the app is quitting
    }
    else {
        if (comThread_) {
            // Disconnects all the signals from the thread
            comThread_->disconnect();

            // If the comthread is running we need to wait
            // until it finishes its task. Otherwise we do not perform
            // logout and leave it to the system to clean up.
            if (comThread_->wait(1000)) {
                // we perform the logout without running the thread
                comThread_->blockingLogout();
                comThread_->deleteLater();
                comThread_ = nullptr;
                deleteLater();
                return true;
            }
        }
        else {
            deleteLater();
            if (client_) {
                delete client_;
                client_ = nullptr;
            }
            return true;
        }
    }
    return false;
}

void ServerComQueue::slotLogoutCheck() {
    Q_ASSERT(state_ == DisabledState);
    // If the comthread is running we need to wait
    // until it finishes its task.

    logoutTryNo_++;
    // Otherwise there is nothing to do!!
    if (logoutTryNo_ > maxLogoutTryNo_) {
        UserMessage::message(UserMessage::ERROR,
                             true,
                             "Could not start logout for server " + server_->fullLongName() +
                                 " because the ServerCom thread cannot be stopped (timeout =" +
                                 std::to_string(logoutInterval_ * maxLogoutTryNo_ / 1000) +
                                 "s has passed). The SeverCom thread must be in a bad state."
                                 "EcFlowUI will terminate!");
        exit(1);
    }

    UiLogS(server_).warn() << "Checking ComThread status to start logout. Tryno=" << logoutTryNo_;

    if (!comThread_->isRunning()) {
        startLogout();
    }
    else {
        QTimer::singleShot(logoutInterval_, this, SLOT(slotLogoutCheck()));
    }
}

void ServerComQueue::startLogout() {
    Q_ASSERT(state_ == DisabledState);
    Q_ASSERT(comThread_->isRunning() == false);
    connect(comThread_, SIGNAL(logoutDone()), this, SLOT(slotLogoutDone()));

    VTask_ptr task = VTask::create(VTask::LogOutTask);
    comThread_->task(task);
}

void ServerComQueue::slotLogoutDone() {
    Q_ASSERT(state_ == DisabledState);
    if (comThread_) {
        Q_ASSERT(comThread_->isRunning() == false);
        comThread_->deleteLater();
        comThread_ = nullptr;
    }
    // notify the server that the queue and the thread is being deleted
    if (client_) {
        delete client_;
        client_ = nullptr;
    }
    server_->queueLoggedOut();
    deleteLater();
}
