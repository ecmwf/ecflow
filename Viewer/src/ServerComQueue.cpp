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
	timeout_(1000),
	state_(SuspendedState), //the queue is enabled but not running
	taskIsBeingFinished_(false),
	taskIsBeingFailed_(false)
{
	timer_=new QTimer(this);
	timer_->setInterval(timeout_);

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

	UserMessage::message(UserMessage::DBG, false, std::string("ServerComQueue::disable"));

	//Clear the current task
	if(current_)
		current_.reset();
}


//This is a special mode to reload the whole ClientInvoker
void ServerComQueue::reset()
{
	if(state_ == DisabledState || state_ == ResetState)
		return;

	//Remove all tasks
	tasks_.clear();

	//Stop the timer
	timer_->stop();

	//This state has the highest priority.
	state_=ResetState;


	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//Detach the thread from the ClientInvoker
	comThread_->detach();

	//The thread cannot be running
	assert(comThread_->isRunning() == false);

	//We send a Reset command to the thread!! This is the only task that is allowed
	//during the reset!!
	VTask_ptr task=VTask::create(VTask::ResetTask);
	tasks_.push_back(task);

	//We start the timer with a shorter interval
	timer_->start(100);
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
		UserMessage::message(UserMessage::DBG, false, std::string("comQueue::start"));

		//If the comthread is running we need to wait
		//until it finishes its task.
		comThread_->wait();

		state_=RunningState;

		UserMessage::message(UserMessage::DBG, false, std::string("comQueue::start start timer"));

		//Starts the timer
		timer_->start(timeout_);
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
	if(state_ != SuspendedState)
	{
		if(!timer_->isActive())
		{
			//we immediately execute the "current" task
			slotRun();
			//and only start the timer after it
			timer_->start(timeout_);
		}
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
	if(state_ == DisabledState ||state_ == SuspendedState )
		return;

	if(taskIsBeingFinished_ || taskIsBeingFailed_)
		return;

	//UserMessage::message(UserMessage::DBG, false, std::string("ServerComQueue::slotRun"));
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

	//We search for the first non-cancelled/aborted task
	while(!tasks_.empty())
	{
		current_=tasks_.front();
		tasks_.pop_front();
		if(current_->status() != VTask::CANCELLED &&
		   current_->status() != VTask::ABORTED )
		{
			break;
		}
	}

	if(!current_)
	{
		timer_->stop();
		return;
	}

	//UserMessage::message(UserMessage::DBG, false,"     --> run task: " +  current_->typeString());

	//Send it to the thread
	comThread_->task(current_);
}

//This slot is called when ComThread finishes its task. At this point the
//thread is not running so it is safe to access the ClientInvoker!
void ServerComQueue::slotTaskFinished()
{
	taskIsBeingFinished_=true;

	UserMessage::message(UserMessage::DBG, false,std::string("ServerComQueue::slotTaskFinished"));

	//We need to leave the load mode
	endReset();

	//If the current task is empty there must have been an error that was
	//handled by the sloTaskFailed slot.
	if(current_)
	{
		VTask_ptr task=current_;
		current_.reset();

		//We notify the server that the task has finished and the results can be accessed.
		server_->clientTaskFinished(task,client_->server_reply());
	}

	taskIsBeingFinished_=false;
}

//This slot is called when the task failed in the ComThread. Right after this signal is emitted
//the thread will finish and and emits the finished() signal that is connected
//to the slotTaskFinished slot.
void ServerComQueue::slotTaskFailed(std::string msg)
{
	taskIsBeingFailed_=true;

	UserMessage::message(UserMessage::DBG, false,std::string("ServerComQueue::slotTaskFailed"));

	//We need to leave the load mode
	endReset();

	assert(current_);

	VTask_ptr task=current_;
	current_.reset();

	//We notify the server that the task has failed
	server_->clientTaskFailed(task,msg);

	taskIsBeingFailed_=false;
}
