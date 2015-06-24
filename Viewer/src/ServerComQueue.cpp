//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerComQueue.hpp"

#include "ClientInvoker.hpp"
#include "ServerComThread.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"

// This class manages the tasks to be sent to the ServerComThread, which controls
// the communication with the ClientInvoker. The ClientInvoker is hidden from the
// ServerHandler. The ServerHandler defines a task and sends it to
// ServerComQueue, which then passes it on to the ClientInvoker. When a task is finished
// ServerComQueue notifies the ServerHandler about it.

ServerComQueue::ServerComQueue(ServerHandler *server,ClientInvoker *client, ServerComThread *comThread) :
	QObject(server),
	server_(server),
	client_(client),
	comThread_(comThread),
	state_(SuspendedState) //the queue is enabled but not running
{
	timer_=new QTimer(this);
	timer_->setInterval(2000);

	connect(timer_,SIGNAL(timeout()),
			this,SLOT(slotRun()));

	//When the ServerComThread finishes it emits a signal that
	//is connected to the queue.
	connect(comThread_, SIGNAL(finished()),
			this, SLOT(slotTaskFinished()));

	//When there is an error in ServerComThread it emits the
	//failed() signal that is connected to the queue.
	connect(comThread_, SIGNAL(failed(std::string)),
			this, SLOT(slotTaskFailed(std::string)));
}

ServerComQueue::~ServerComQueue()
{
	state_=DisabledState;

	//Stop the timer
	timer_->stop();

	//Empty the tasks
	tasks_.clear();

	//Disconnects all the signals from the thread
	comThread_->disconnect(0,this);

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//Detach the thread from the ClientInvoker
	comThread_->detach();

	//Send a logout task
	VTask_ptr task=VTask::create(VTask::LogOutTask);
	comThread_->task(task);

	//Wait unit the logout finishes
	comThread_->wait();

	delete comThread_;
}

void ServerComQueue::enable()
{
	state_=SuspendedState;
	start();
}

void ServerComQueue::disable()
{
	if(state_ == DisabledState)
		return;

	state_=DisabledState;

	//Remove all tasks
	tasks_.clear();

	//Stop the timer
	timer_->stop();

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//Clear the current task
	if(current_)
		current_.reset();
}


//This is a special mode to reload the whole ClientInvoker
void ServerComQueue::reset()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	//This state has the highest priority.
	state_=ResetState;

	//Remove all tasks
	tasks_.clear();

	//Stop the timer
	timer_->stop();

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//Detach the thread from the ClientInvoker
	comThread_->detach();

	//The thread cannot be running
	assert(comThread_->isRunning() == false);

	//We send a Reset command straight to the thread!!
	VTask_ptr task=VTask::create(VTask::ResetTask);
	current_=task;

	comThread_->task(current_);

	//The queue is still stopped and does not accept any tasks until the reset finishes.
}

void ServerComQueue::endReset()
{
	if(state_ == ResetState)
	{
		state_=SuspendedState;
		start();
	}
}


//When the queue is started:
// -it is ready to accept tasks
// -its timer is running
void ServerComQueue::start()
{
	if(state_ != DisabledState && state_ != ResetState)
	{
		//If the comthread is running we need to wait
		//until it finishes its task.
		comThread_->wait();

		state_=RunningState;

		//Starts the timer
		timer_->start();
	}
}

//The queue contents remains the same but the timer is stopped. Until start() is
//called nothing will be submitted to the queue.
void ServerComQueue::suspend()
{
	if(state_ != DisabledState &&
	   state_ != ResetState)
	{
		state_=SuspendedState;
		timer_->stop();
	}
}

void ServerComQueue::addTask(VTask_ptr task)
{
	if(!task)
		return;

	if(state_ == DisabledState || state_ == ResetState ||
	  (task && task->type() ==VTask::ResetTask) )
		return;

	tasks_.push_back(task);
	if(!timer_->isActive() && state_ != SuspendedState)
	{
		timer_->start(0);
	}
}

void ServerComQueue::addNewsTask()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	VTask_ptr task=VTask::create(VTask::NewsTask);
	addTask(task);
}

void ServerComQueue::addSyncTask()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	VTask_ptr task=VTask::create(VTask::SyncTask);
	addTask(task);
}

void ServerComQueue::addSuiteListTask()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	VTask_ptr task=VTask::create(VTask::SuiteListTask);
	addTask(task);
}

void ServerComQueue::addSuiteAutoRegisterTask()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	VTask_ptr task=VTask::create(VTask::SuiteAutoRegisterTask);
	addTask(task);
}

void ServerComQueue::slotRun()
{
	if(state_ == DisabledState || state_ == ResetState ||state_ == SuspendedState )
		return;

	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComQueue::run"));
	//UserMessage::message(UserMessage::DBG, false, std::string("     --> number of tasks: " + boost::lexical_cast<std::string>(tasks_.size()) ));
	//for(std::deque<VTask_ptr>::const_iterator it=tasks_.begin(); it != tasks_.end(); it++)
	//{
	//	UserMessage::message(UserMessage::DBG, false,"        -task: " + (*it)->typeString());
	//}

	if(tasks_.empty())
	{
		//UserMessage::message(UserMessage::DBG, false, std::string("     --> stop timer"));
		timer_->stop();
		return;
	}

	if(current_)
	{
		//UserMessage::message(UserMessage::DBG, false, std::string("     --> processing reply from previous task"));
		return;
	}

	if(comThread_->isRunning())
	{
		//UserMessage::message(UserMessage::DBG, false, std::string("     --> thread is active"));
		return;
	}

	current_=tasks_.front();
	tasks_.pop_front();

	//UserMessage::message(UserMessage::DBG, false,"     --> run task: " +  current_->typeString());

	//Send it to the thread
	comThread_->task(current_);
}

//This slot is called when ComThread finishes its task. At this point the
//thread is not running so it is safe to access the ClientInvoker!
void ServerComQueue::slotTaskFinished()
{
	UserMessage::message(UserMessage::DBG, false,std::string("ServerComQueue::slotTaskFinished"));

	//We need to leave the load mode
	endReset();

	//If the current task is empty there must have been an error that was
	//handled by the sloTaskFailed slot.
	if(!current_)
		return;

	//We notify the server that the task has finished and the results can be accessed.
	server_->clientTaskFinished(current_,client_->server_reply());

	//We do not need the current task any longer.
	if(current_)
		current_.reset();
}

//This slot is called when the task failed in the ComThread. Right after this signal is emitted
//the thread will finish and and emits the finished() signal that is connected
//to the slotTaskFinished slot.
void ServerComQueue::slotTaskFailed(std::string msg)
{
	UserMessage::message(UserMessage::DBG, false,std::string("ServerComQueue::slotTaskFailed"));

	//We need to leave the load mode
	endReset();

	//We notify the server that the task has failed
	server_->clientTaskFailed(current_,msg);

	//current_->reply()->errorText(msg);
	//current_->status(VTask::ABORTED);

	//We do not need the current task any longer.
	if(current_)
		current_.reset();
}
