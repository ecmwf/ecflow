//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerHandler.hpp"

#include "Defs.hpp"
#include "ClientInvoker.hpp"
#include "File.hpp"
#include "NodeFwd.hpp"
#include "ArgvCreator.hpp"
#include "Str.hpp"

#include "ChangeNotify.hpp"
#include "ConnectState.hpp"
#include "DirectoryHandler.hpp"
#include "MainWindow.hpp"
#include "NodeObserver.hpp"
#include "SessionHandler.hpp"
#include "ServerComQueue.hpp"
#include "ServerDefsAccess.hpp"
#include "ServerObserver.hpp"
#include "ServerComObserver.hpp"
#include "ShellCommand.hpp"
#include "SuiteFilter.hpp"
#include "UiLog.hpp"
#include "UIDebug.hpp"
#include "UpdateTimer.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"
#include "VTaskObserver.hpp"

#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QMetaType>

#include <iostream>
#include <algorithm>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/ip/host_name.hpp>

std::vector<ServerHandler*> ServerHandler::servers_;
std::string ServerHandler::localHostName_;

//#define __UI_SERVEROBSERVER_DEBUG
//#define __UI_SERVERCOMOBSERVER_DEBUG
#define __UI_SERVERUPDATE_DEBUG

ServerHandler::ServerHandler(const std::string& name,const std::string& host, const std::string& port) :
   name_(name),
   host_(host),
   port_(port),
   client_(0),
   updating_(false),
   communicating_(false),
   suiteFilter_(new SuiteFilter()),
   comQueue_(0),
   activity_(NoActivity),
   connectState_(new ConnectState()),
   prevServerState_(SState::RUNNING),
   conf_(0)
{
	if(localHostName_.empty())
	{
		localHostName_=boost::asio::ip::host_name();
	}

	//Create longname
	longName_=host_ + "@" + port_;

	conf_=new VServerSettings(this);

	//Create the client invoker. At this point it is empty.
    try
    {
        client_=new ClientInvoker(host,port);
    }
    catch(std::exception& e)
    {
        UiLog().err() << "Could not create ClientInvoker for host=" << host <<
                         " port= " << port << ". " <<  e.what();
        client_=0;
    }

	client_->set_retry_connection_period(1);
	client_->set_throw_on_error(true);

	//Create the vnode root. This will represent the node tree in the viewer, but
	//at this point it is empty.
	vRoot_=new VServer(this);

	//Connect up the timer for refreshing the server info. The timer has not
	//started yet.

    refreshTimer_=new UpdateTimer(this);
    connect(refreshTimer_, SIGNAL(timeout()),
			this, SLOT(refreshServerInfo()));


	//We will need to pass various non-Qt types via signals and slots for error messages.
	//So we need to register these types.
	if(servers_.empty())
	{
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<QList<ecf::Aspect::Type> >("QList<ecf::Aspect::Type>");
		qRegisterMetaType<std::vector<ecf::Aspect::Type> >("std::vector<ecf::Aspect::Type>");
	}

	//Add this instance to the servers_ list.
	servers_.push_back(this);

	//NOTE: we may not always want to create a thread here because of resource
	// issues; another strategy would be to create threads on demand, only
	// when server communication is about to start.

	//Create the queue for the tasks to be sent to the client (via the ServerComThread)! It will
    //create and take ownership of the ServerComThread. At this point the queue has not started yet.
    comQueue_=new ServerComQueue (this,client_);

	//Load settings
	loadConf();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// At this point nothing is running or active!!!!

	//Indicate that we start an init (initial load)
	//activity_=LoadActivity;

	//Try to connect to the server and load the defs etc. This might fail!
	reset();
}

ServerHandler::~ServerHandler()
{
	//Save setings
	saveConf();

	//Notify the observers
    broadcast(&ServerObserver::notifyServerDelete);

	//The queue must be deleted before the client, since the thread might
	//be running a job on the client!!
	if (comQueue_)
		delete comQueue_;

	//Remove itself from the server vector
	std::vector<ServerHandler*>::iterator it=std::find(servers_.begin(),servers_.end(),this);
	if(it != servers_.end())
		servers_.erase(it);

	delete vRoot_;
	delete connectState_;
	delete suiteFilter_;

	//The safest is to delete the client in the end
	if(client_)
		delete client_;

	delete conf_;
}

//-----------------------------------------------
// Refresh/update
//-----------------------------------------------

bool ServerHandler::updateInfo(int& basePeriod,int& currentPeriod,int &drift,int& toNext)
{
    if(!refreshTimer_->isActive())
        return false;

    toNext=secsTillNextRefresh();
    basePeriod=conf_->intValue(VServerSettings::UpdateRate);
    currentPeriod=refreshTimer_->interval()/1000;
    drift=-1;
    if(conf_->boolValue(VServerSettings::AdaptiveUpdate))
    {
        drift=conf_->intValue(VServerSettings::AdaptiveUpdateIncrement);
    }

    return true;
}

int ServerHandler::secsSinceLastRefresh() const
{
    return static_cast<int>(lastRefresh_.secsTo(QDateTime::currentDateTime()));
}

int ServerHandler::secsTillNextRefresh() const
{
     if(refreshTimer_->isActive())
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
         return refreshTimer_->remainingTime()/1000;
#else
         return 0;
#endif
     return -1;
}

void ServerHandler::stopRefreshTimer()
{
    refreshTimer_->stop();
#ifdef __UI_SERVERUPDATE_DEBUG
    UiLog(this).dbg() << "ServerHandler::stopRefreshTimer -->";
#endif
    broadcast(&ServerComObserver::notifyRefreshTimerStopped);
}

void ServerHandler::startRefreshTimer()
{
    UiLog(this).dbg() << "ServerHandler::startRefreshTimer -->";

    if(!conf_->boolValue(VServerSettings::AutoUpdate))
    {
        return;
    }

    //If we are not connected to the server the
	//timer should not run.
	if(connectState_->state() == ConnectState::Disconnected)
		return;

    if(!refreshTimer_->isActive())
	{
        int rate=conf_->intValue(VServerSettings::UpdateRate);
        if(rate <=0) rate=1;
        refreshTimer_->setInterval(rate*1000);
        refreshTimer_->start();
        broadcast(&ServerComObserver::notifyRefreshTimerStarted);
	}

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLog(this).dbg() << " refreshTimer interval: " <<  refreshTimer_->interval();
#endif
}

void ServerHandler::updateRefreshTimer()
{
    UiLog(this).dbg() << "ServerHandler::updateRefreshTimer -->";

    if(!conf_->boolValue(VServerSettings::AutoUpdate))
    {
        stopRefreshTimer();
        return;
    }

    //If we are not connected to the server the
	//timer should not run.
	if(connectState_->state() == ConnectState::Disconnected)
		return;

    int rate=conf_->intValue(VServerSettings::UpdateRate);
    if(rate <=0) rate=1;

    if(refreshTimer_->isActive())
	{
        refreshTimer_->stop();
    }

    refreshTimer_->setInterval(rate*1000);
    refreshTimer_->start();
    broadcast(&ServerComObserver::notifyRefreshTimerChanged);

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLog(this).dbg() << " refreshTimer interval: " << refreshTimer_->interval();
#endif

}

void ServerHandler::driftRefreshTimer()
{
    if(!conf_->boolValue(VServerSettings::AutoUpdate))
    {
        return;
    }

    //We increase the update frequency
    if(activity_ != LoadActivity &&
       conf_->boolValue(VServerSettings::AdaptiveUpdate))
    {
#ifdef __UI_SERVERUPDATE_DEBUG
        UiLog(this).dbg() << "driftRefreshTimer -->";
#endif

        int rate=conf_->intValue(VServerSettings::UpdateRate);
        int baseDelta=conf_->intValue(VServerSettings::AdaptiveUpdateIncrement);
        int delta=baseDelta;  //linear mode

        //doubling mode
        if(conf_->stringValue(VServerSettings::AdaptiveUpdateMode) == "doubling")
        {
            delta=refreshTimer_->interval()/1000-rate;
            if(delta==0)
                delta=baseDelta;
        }
        //x modes
        else
        {
            float f=conf_->stringValue(VServerSettings::AdaptiveUpdateMode).toFloat();
            if(f >= 1. && f <=5.)
            {
                delta=((10*(refreshTimer_->interval()/1000-rate))*(f-1))/10;
                if(delta==0)
                    delta=baseDelta;
            }
        }

        refreshTimer_->drift(delta,conf_->intValue(VServerSettings::MaxAdaptiveUpdateRate));

        broadcast(&ServerComObserver::notifyRefreshTimerChanged);
    }

#ifdef __UI_SERVERUPDATE_DEBUG
    UiLog(this).dbg() << "driftRefreshTimer interval: " << refreshTimer_->interval();
#endif

}

//returns true if the current total (drifted) period is within the maximum allowed
bool ServerHandler::checkRefreshTimerDrift() const
{
    if(!conf_->boolValue(VServerSettings::AutoUpdate))
    {
        return true;
    }

    if(activity_ != LoadActivity &&
       conf_->boolValue(VServerSettings::AdaptiveUpdate))
    {
        return (refreshTimer_->interval()*1000 <
                conf_->intValue(VServerSettings::MaxAdaptiveUpdateRate)*60);
    }
    return true;
}

//mark that a refresh request was sent to the queue
void ServerHandler::refreshScheduled()
{
    lastRefresh_=QDateTime::currentDateTime();
    broadcast(&ServerComObserver::notifyRefreshScheduled);
}

//mark that a refresh request was sent to the queue
void ServerHandler::refreshFinished()
{
    broadcast(&ServerComObserver::notifyRefreshFinished);
}

void ServerHandler::setActivity(Activity ac)
{
	activity_=ac;
	broadcast(&ServerObserver::notifyServerActivityChanged);
}

ServerHandler* ServerHandler::addServer(const std::string& name,const std::string& host, const std::string& port)
{
	ServerHandler* sh=new ServerHandler(name,host,port);
    //Without the clinetinvoker we cannot use the serverhandler
    if(!sh->client_)
    {
        delete sh;
        sh=0;
    }
    return sh;
}

void ServerHandler::removeServer(ServerHandler* server)
{
	std::vector<ServerHandler*>::iterator it=std::find(servers_.begin(), servers_.end(),server);
	if(it != servers_.end())
	{
		ServerHandler *s=*it;
		servers_.erase(it);
		delete s;
	}
}

ServerHandler* ServerHandler::findServer(const std::string &alias)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
	{
		ServerHandler *s=*it;

		if (s->name() == alias)
		{
			return s;
		}
	}
	return NULL; // did not find it
}



//This function can be called many times so we need to avoid locking the mutex.
SState::State ServerHandler::serverState()
{
	if(connectState_->state() != ConnectState::Normal || activity() == LoadActivity)
	{
		prevServerState_= SState::RUNNING;

	}
	//If we are here we can be sure that the defs pointer is not being deleted!! We
	//access it without locking the mutex!!!
	else
	{
		//If get the defs can be 100% sure that it is not being deleted!! So we can
		//access it without locking the mutex!!!
		defs_ptr d=safelyAccessSimpleDefsMembers();
		if(d && d.get())
		{
			prevServerState_= d->set_server().get_state();
			return prevServerState_;
		}
	}

	return prevServerState_;
}

//This function can be called many times so we need to avoid locking the mutex.
NState::State ServerHandler::state(bool& suspended)
{
	if(connectState_->state() != ConnectState::Normal || activity() == LoadActivity)
		return NState::UNKNOWN;

	suspended=false;

	defs_ptr d=safelyAccessSimpleDefsMembers();
	if(d && d.get())
	{
		suspended=d->isSuspended();
		return d->state();
	}

	return NState::UNKNOWN;
}

defs_ptr ServerHandler::defs()
{
	defs_ptr null;

	if(client_)
	{
		return client_->defs();
	}
	else
	{
		return null;
	}
}

defs_ptr ServerHandler::safelyAccessSimpleDefsMembers()
{
	defs_ptr null;

	//The defs might be deleted during reset so it cannot be accessed.
	if(activity_ == LoadActivity)
	{
		return null;
	}
	//Otherwise it is safe to access certain non-vector members
	else if(client_)
	{
		return client_->defs();
	}
	else
	{
		return null;
	}
}

//-------------------------------------------------------------
// Run client tasks.
//
// The preferred way to run client tasks is to define and add a task to the queue. The
// queue will manage the task and will send it to the ClientInvoker. When the task
// finishes the ServerHandler::clientTaskFinished method is called where the
// result/reply can be processed.
//--------------------------------------------------------------

//It is protected! Practically it means we
//we can only run it through CommandHandler!!!
void ServerHandler::runCommand(const std::vector<std::string>& cmd)
{
    //Shell command - we should not reach this point
    if(cmd.size() > 0 && cmd[0]=="sh")
    {
        UI_ASSERT(0,"cmd=" << cmd[0]);
        return;
    }

    //ecflow_client command
    if(connectState_->state() == ConnectState::Disconnected)
		return;

	VTask_ptr task=VTask::create(VTask::CommandTask);
	task->command(cmd);
	comQueue_->addTask(task);
}

void ServerHandler::run(VTask_ptr task)
{
	if(connectState_->state() == ConnectState::Disconnected)
		return;

	switch(task->type())
	{
	case VTask::ScriptTask:
		return script(task);
		break;
	case VTask::JobTask:
		return job(task);
		break;
	case VTask::OutputTask:
		return jobout(task);
		break;
	case VTask::ManualTask:
		return manual(task);
		break;
	case VTask::HistoryTask:
	case VTask::MessageTask:
	case VTask::StatsTask:
	case VTask::ScriptPreprocTask:
	case VTask::ScriptEditTask:
	case VTask::ScriptSubmitTask:
	case VTask::SuiteListTask:
    case VTask::ZombieListTask:
    case VTask::ZombieCommandTask:
		comQueue_->addTask(task);
		break;
	default:
		//If we are here we have an unhandled task type.
		task->status(VTask::REJECTED);
		break;
	}
}

void ServerHandler::script(VTask_ptr task)
{
	/*static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";*/

	task->param("clientPar","script");
	comQueue_->addTask(task);
}

void ServerHandler::job(VTask_ptr task)
{
	/*static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";*/

	task->param("clientPar","job");
	comQueue_->addTask(task);
}

void ServerHandler::jobout(VTask_ptr task)
{
	//static std::string errText="no job output...";

	task->param("clientPar","jobout");
	comQueue_->addTask(task);
}

void ServerHandler::manual(VTask_ptr task)
{
	//std::string errText="no manual ...";
	task->param("clientPar","manual");
	comQueue_->addTask(task);
}

//The core refresh function. That should be the only one directly accessing the queue.
void ServerHandler::refreshInternal()
{
    //We add and refresh task to the queue. On startup this function can be called
    //before the comQueue_ was created so we need to check if it exists.
    if(comQueue_)
    {
        comQueue_->addNewsTask();
    }
}

//The user initiated a refresh - using the toolbar/menu buttons or
//called after a user command was issued
void ServerHandler::refresh()
{
    refreshInternal();

    //Reset the timer to its original value (i.e. remove the drift)
    updateRefreshTimer();
}


//This slot is called by the timer regularly to get news from the server.
void ServerHandler::refreshServerInfo()
{
    UiLog(this).dbg() << "Auto refreshing server";
    refreshInternal();

    //We reduce the update frequency
    driftRefreshTimer();
}

//======================================================================================
// Manages node changes.
//======================================================================================

//This slot is called when a node changes.
void ServerHandler::slotNodeChanged(const Node* nc,std::vector<ecf::Aspect::Type> aspect)
{
    UiLog(this).dbg() << "ServerHandler::slotNodeChanged - node: " + nc->name();

    //for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    //UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + boost::lexical_cast<std::string>(*it));

	//This can happen if we initiated a reset while we sync in the thread
	if(vRoot_->isEmpty())
	{
        UiLog(this).dbg() << " tree is empty - no change applied!";
		return;
	}

	VNode* vn=vRoot_->toVNode(nc);

	//We must have this VNode
	assert(vn != NULL);

	//Begin update for the VNode
	VNodeChange change;
	vRoot_->beginUpdate(vn,aspect,change);

	//TODO: what about the infopanel or breadcrumbs??????
	if(change.ignore_)
	{
        UiLog(this).dbg() << " update ignored";
		return;
	}
	else
	{
		//Notify the observers
		broadcast(&NodeObserver::notifyBeginNodeChange,vn,aspect,change);

		//End update for the VNode
		vRoot_->endUpdate(vn,aspect,change);

		//Notify the observers
		broadcast(&NodeObserver::notifyEndNodeChange,vn,aspect,change);

        UiLog(this).dbg() << " update applied";
	}
}

void ServerHandler::addNodeObserver(NodeObserver *obs)
{
	std::vector<NodeObserver*>::iterator it=std::find(nodeObservers_.begin(),nodeObservers_.end(),obs);
	if(it == nodeObservers_.end())
	{
		nodeObservers_.push_back(obs);
	}
}

void ServerHandler::removeNodeObserver(NodeObserver *obs)
{
	std::vector<NodeObserver*>::iterator it=std::find(nodeObservers_.begin(),nodeObservers_.end(),obs);
	if(it != nodeObservers_.end())
	{
		nodeObservers_.erase(it);
	}
}

void ServerHandler::broadcast(NoMethod proc,const VNode* node)
{
	for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); ++it)
		((*it)->*proc)(node);
}

void ServerHandler::broadcast(NoMethodV1 proc,const VNode* node,const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange& change)
{       
    //When the observers are being notified (in a loop) they might
    //want to remove themselves from the observer list. This will cause a crash. To avoid
    //this we create a copy of the observers and use it in the notification loop.
    std::vector<NodeObserver*> nObsCopy=nodeObservers_;

    for(std::vector<NodeObserver*>::const_iterator it=nObsCopy.begin(); it != nObsCopy.end(); ++it)
    {
        ((*it)->*proc)(node,aspect,change);
    }

#if 0
    for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); ++it)
		((*it)->*proc)(node,aspect,change);
#endif
}

//---------------------------------------------------------------------------
// Manages Defs changes and desf observers. Defs observers are notified when
// there is a change.
//---------------------------------------------------------------------------

//This slot is called when the Defs change.
void ServerHandler::slotDefsChanged(std::vector<ecf::Aspect::Type> aspect)
{
    UiLog().dbg() << "ServerHandler::slotDefsChanged -->";
    //for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    //	UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + boost::lexical_cast<std::string>(*it));

	//Begin update for the VNode
    //VNodeChange change;
	vRoot_->beginUpdate(aspect);

	//Notify the observers
	//broadcast(&NodeObserver::notifyBeginNodeChange,vn,aspect,change);

	//End update for the VNode
	//vRoot_->endUpdate(vn,aspect,change);

	//Notify the observers
	//broadcast(&NodeObserver::notifyEndNodeChange,vn,aspect,change);

	//UserMessage::message(UserMessage::DBG, false," --> Update applied");

	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); ++it)
		(*it)->notifyDefsChanged(this,aspect);
}

void ServerHandler::addServerObserver(ServerObserver *obs)
{
	std::vector<ServerObserver*>::iterator it=std::find(serverObservers_.begin(),serverObservers_.end(),obs);
	if(it == serverObservers_.end())
	{
        serverObservers_.push_back(obs);
#ifdef __UI_SERVEROBSERVER_DEBUG
        UiLog(this).dbg() << "ServerHandler::addServerObserver -->  " << obs;
#endif
	}
}

void ServerHandler::removeServerObserver(ServerObserver *obs)
{
#ifdef __UI_SERVEROBSERVER_DEBUG
    UI_FUNCTION_LOG_S(this)
#endif
    std::vector<ServerObserver*>::iterator it=std::find(serverObservers_.begin(),serverObservers_.end(),obs);
	if(it != serverObservers_.end())
	{
		serverObservers_.erase(it);
#ifdef __UI_SERVEROBSERVER_DEBUG
        UiLog(this).dbg() << " remove: " << obs;
#endif
	}
}

void ServerHandler::broadcast(SoMethod proc)
{
#ifdef __UI_SERVEROBSERVER_DEBUG
    UI_FUNCTION_LOG_S(this)
#endif

    bool checkExistence=true;

    //When the observers are being notified (in a loop) they might
	//want to remove themselves from the observer list. This will cause a crash. To avoid
	//this we create a copy of the observers and use it in the notification loop.
	std::vector<ServerObserver*> sObsCopy=serverObservers_;

	for(std::vector<ServerObserver*>::const_iterator it=sObsCopy.begin(); it != sObsCopy.end(); ++it)
	{
		//We need to check if the given observer is still in the original list. When we delete the server, due to
		//dependencies it is possible that the observer is already deleted at this point.
		if(!checkExistence || std::find(serverObservers_.begin(),serverObservers_.end(),*it) != serverObservers_.end())
        {
            ((*it)->*proc)(this);
        }
	}
}


void ServerHandler::broadcast(SoMethodV1 proc,const VServerChange& ch)
{
	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); ++it)
		((*it)->*proc)(this,ch);
}

//------------------------------------------------
// ServerComObserver
//------------------------------------------------

void ServerHandler::addServerComObserver(ServerComObserver *obs)
{
    std::vector<ServerComObserver*>::iterator it=std::find(serverComObservers_.begin(),serverComObservers_.end(),obs);
    if(it == serverComObservers_.end())
    {
        serverComObservers_.push_back(obs);
#ifdef __UI_SERVERCOMOBSERVER_DEBUG
        UiLog(this).dbg() << "ServerHandler::addServerComObserver -->  " << obs;
#endif
    }
}

void ServerHandler::removeServerComObserver(ServerComObserver *obs)
{
    std::vector<ServerComObserver*>::iterator it=std::find(serverComObservers_.begin(),serverComObservers_.end(),obs);
    if(it != serverComObservers_.end())
    {
        serverComObservers_.erase(it);
        #ifdef __UI_SERVERCOMOBSERVER_DEBUG
            UiLog(this).dbg() << "ServerHandler::removeServerComObserver --> " << obs;
        #endif
    }
}

void ServerHandler::broadcast(SocMethod proc)
{
    bool checkExistence=true;

    //When the observers are being notified (in a loop) they might
    //want to remove themselves from the observer list. This will cause a crash. To avoid
    //this we create a copy of the observers and use it in the notification loop.
    std::vector<ServerComObserver*> sObsCopy=serverComObservers_;

    for(std::vector<ServerComObserver*>::const_iterator it=sObsCopy.begin(); it != sObsCopy.end(); ++it)
    {
        //We need to check if the given observer is still in the original list. When we delete the server, due to
        //dependencies it is possible that the observer is already deleted at this point.
        if(!checkExistence || std::find(serverComObservers_.begin(),serverComObservers_.end(),*it) != serverComObservers_.end())
            ((*it)->*proc)(this);
    }
}


//-------------------------------------------------------------------
// This slot is called when the comThread finished the given task!!
//-------------------------------------------------------------------

//There was a drastic change during the SYNC! As a safety measure we need to clear
//the tree. We will rebuild it when the SYNC finishes.
void ServerHandler::slotRescanNeed()
{
	clearTree();
}

void ServerHandler::clientTaskFinished(VTask_ptr task,const ServerReply& serverReply)
{
    UiLog(this).dbg() << "ServerHandler::clientTaskFinished -->";

    //The status of the tasks sent from the info panel must be properly set to
    //FINISHED!! Only that will notify the observers about the change!!

	//See which type of task finished. What we do now will depend on that.
	switch(task->type())
	{
		case VTask::CommandTask:
		{
			// a command was sent - we should now check whether there have been
			// any updates on the server (there should have been, because we
			// just did something!)

            UiLog(this).dbg() << " command finished - send SYNC command";
			//comQueue_->addNewsTask();
			comQueue_->addSyncTask();
            updateRefreshTimer();
			break;
		}
		case VTask::NewsTask:
		{
			// we've just asked the server if anything has changed - has it?

            refreshFinished();

			switch (serverReply.get_news())
			{
				case ServerReply::NO_NEWS:
				{
					// no news, nothing to do
                    UiLog(this).dbg() << " NO_NEWS";

					//If we just regained the connection we need to reset
					if(connectionGained())
					{
						reset();
					}
					break;
				}

				case ServerReply::NEWS:
				{
					// yes, something's changed - synchronise with the server

					//If we just regained the connection we need to reset
                    UiLog(this).dbg() << " NEWS - send SYNC command";
					if(connectionGained())
					{
						reset();
					}
					else
					{
						comQueue_->addSyncTask();
					}
					break;
				}

				case ServerReply::DO_FULL_SYNC:
				{
					// yes, a lot of things have changed - we need to reset!!!!!!                       
                    UiLog(this).dbg() << " DO_FULL_SYNC - will reset";
					connectionGained();
					reset();
					break;
				}

				default:
				{
					break;
				}
			}
			break;
		}
		case VTask::SyncTask:
		{
            UiLog(this).dbg() << " sync finished";

			//This typically happens when a suite is added/removed
			if(serverReply.full_sync() || vRoot_->isEmpty())
			{
                UiLog(this).dbg() << " full sync requested - rescanTree";

                //This will update the suites + restart the timer
				rescanTree();

#if 0
                {
                    ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
                    defs_ptr defs = defsAccess.defs();
                    if(defs)
                    {
                        std::cout << defs;
                    }
                }
#endif

			}
			else
			{
                broadcast(&ServerObserver::notifyEndServerSync);
			}
			break;
		}

		case VTask::ResetTask:
		{
			//If not yet connected but the sync task was successful
			resetFinished();
			break;
		}

		case VTask::ScriptTask:
		case VTask::ManualTask:
		case VTask::HistoryTask:
		case VTask::JobTask:	
		{
			task->reply()->fileReadMode(VReply::ServerReadMode);
            task->reply()->setText(serverReply.get_string());
            task->reply()->setReadTruncatedTo(truncatedLinesFromServer(task->reply()->text()));
			task->status(VTask::FINISHED);
			break;
		}

        case VTask::OutputTask:
        {
            task->reply()->fileReadMode(VReply::ServerReadMode);

            std::string err;
            VFile_ptr f(VFile::create(false));
            f->setFetchMode(VFile::ServerFetchMode);
            f->setFetchModeStr("fetched from server " + name());
            f->setSourcePath(task->reply()->fileName());
            f->setTruncatedTo(truncatedLinesFromServer(serverReply.get_string()));
            f->write(serverReply.get_string(),err);
            task->reply()->tmpFile(f);

            task->status(VTask::FINISHED);
            break;
        }

        case VTask::MessageTask:
		{
			task->reply()->setTextVec(serverReply.get_string_vec());
			task->status(VTask::FINISHED);
			break;
		}

		case VTask::StatsTask:
		{
			std::stringstream ss;
			serverReply.stats().show(ss);
			task->reply()->text(ss.str());
			task->status(VTask::FINISHED);
			break;
		}

		case VTask::ScriptPreprocTask:
		case VTask::ScriptEditTask:
		{
			task->reply()->text(serverReply.get_string());
			task->status(VTask::FINISHED);
			break;
		}
		case VTask::ScriptSubmitTask:
		{
            UiLog(this).dbg() << " script submit finished - send NEWS command";

			task->reply()->text(serverReply.get_string());
			task->status(VTask::FINISHED);

			//Submitting the task was successful - we should now get updates from the server			
            refresh();
			break;
		}

		case VTask::SuiteListTask:
		{
			//Update the suite filter with the list of suites actually loaded onto the server.
            //If the suitefilter is enabled this might have only a subset of it in our tree
            updateSuiteFilterWithLoaded(serverReply.get_string_vec());
            task->status(VTask::FINISHED);
			break;
		}

		case VTask::ZombieListTask:
		{
			task->reply()->zombies(serverReply.zombies());
			task->status(VTask::FINISHED);
			break;
		}

		default:
            task->status(VTask::FINISHED);
            break;

	}                                             
}

//-------------------------------------------------------------------
// This slot is called when the comThread failed the given task!!
//-------------------------------------------------------------------

void ServerHandler::clientTaskFailed(VTask_ptr task,const std::string& errMsg)
{
    UiLog(this).dbg() << "ServerHandler::clientTaskFailed -->";

	//TODO: suite filter  + ci_ observers

	//See which type of task finished. What we do now will depend on that.
	switch(task->type())
	{
		case VTask::SyncTask:
			connectionLost(errMsg);
			break;

		//The initialisation failed
		case VTask::ResetTask:
		{
			resetFailed(errMsg);
			break;
		}
		case VTask::NewsTask:
        {
            connectionLost(errMsg);
            refreshFinished();
            break;
        }
        case VTask::StatsTask:
		{
			connectionLost(errMsg);
			break;
		}
		case VTask::CommandTask:
		{
			task->reply()->setErrorText(errMsg);
			task->status(VTask::ABORTED);
			UserMessage::message(UserMessage::WARN, true, errMsg);
			break;
		}
		default:
			task->reply()->setErrorText(errMsg);
			task->status(VTask::ABORTED);
			break;
	}
}

void ServerHandler::connectionLost(const std::string& errMsg)
{
	connectState_->state(ConnectState::Lost);
	connectState_->errorMessage(errMsg);
	broadcast(&ServerObserver::notifyServerConnectState);
}

bool ServerHandler::connectionGained()
{
	if(connectState_->state() != ConnectState::Normal)
	{
		connectState_->state(ConnectState::Normal);
		broadcast(&ServerObserver::notifyServerConnectState);
		return true;
	}
	return false;

}

void ServerHandler::disconnectServer()
{
	if(connectState_->state() != ConnectState::Disconnected)
	{
		connectState_->state(ConnectState::Disconnected);
		broadcast(&ServerObserver::notifyServerConnectState);

		//Stop the queue
		comQueue_->disable();

		//Stop the timer
		stopRefreshTimer();
	}
}

void ServerHandler::connectServer()
{
	if(connectState_->state() == ConnectState::Disconnected)
	{
		//Start the queue
		comQueue_->enable();

		//Start the timer
		startRefreshTimer();

		//Try to get the news
        refreshInternal();

		//TODO: attach the observers!!!!
	}
}

void ServerHandler::reset()
{
    UiLog(this).dbg() << "ServerHandler::reset -->";

	//---------------------------------
	// First part of reset: clearing
	//---------------------------------

	if(comQueue_->prepareReset())
	{
		//Stop the timer
		stopRefreshTimer();

		// First part of reset: clear the tree
		clearTree();

		// Second part of reset: loading

		//Indicate that we reload the defs
		setActivity(LoadActivity);

        //mark the current moment as last refresh
        lastRefresh_=QDateTime::currentDateTime();

		//NOTE: at this point the queue is not running but reset() will start it.
		//While the queue is in reset mode it does not accept tasks.
		comQueue_->reset();
	}
	else
	{
        UiLog(this).dbg() << " skip reset";
	}
}

//The reset was successful
void ServerHandler::resetFinished()
{
	setActivity(NoActivity);

	//Set server host and port in defs. It is used to find the server of
	//a given node in the viewer.
	{
		ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction

		defs_ptr defs = defsAccess.defs();
		if(defs != NULL)
		{
			ServerState& st=defs->set_server();
			st.hostPort(std::make_pair(host_,port_));
		}
	}

	//Create an object to inform the observers about the change
	VServerChange change;

	//Begin the full scan to get the tree. This call does not actually
	//run the scan but counts how many suits will be available.
	vRoot_->beginScan(change);

	//Notify the observers that the scan has started
	broadcast(&ServerObserver::notifyBeginServerScan,change);

	//Finish full scan
	vRoot_->endScan();

    //assert(change.suiteNum_ == vRoot_->numOfChildren());

	//Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

	//The suites might have been changed
	updateSuiteFilter();

	//Restart the timer
	startRefreshTimer();

	//Set the connection state
	if(connectState_->state() != ConnectState::Normal)
	{
		connectState_->state(ConnectState::Normal);
		broadcast(&ServerObserver::notifyServerConnectState);
	}
}

//The reset failed and we could not connect to the server, e.g. because the the server
//may be down, or there is a network error, or the authorisation is missing.
void ServerHandler::resetFailed(const std::string& errMsg)
{
	//This status is indicated by the connectStat_. Each object needs to be aware of it
	//and do its tasks accordingly.

    //Create an object to inform the observers about the change
    VServerChange change;
    //Notify the observers that the scan has started
    broadcast(&ServerObserver::notifyBeginServerScan,change);
    //Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

	connectState_->state(ConnectState::Lost);
	connectState_->errorMessage(errMsg);
	setActivity(NoActivity);

	broadcast(&ServerObserver::notifyServerConnectState);

	//Restart the timer
	startRefreshTimer();
}

//This function must be called during a SYNC!!!!!!!!

void ServerHandler::clearTree()
{
    UiLog(this).dbg() << "ServerHandler::clearTree -->";

	if(!vRoot_->isEmpty())
	{
		//Notify observers that the clear is about to begin
        broadcast(&ServerObserver::notifyBeginServerClear);

		//Clear vnode
		vRoot_->clear();

		//Notify observers that the clear ended
        broadcast(&ServerObserver::notifyEndServerClear);
	}

    UiLog(this).dbg() << " <-- clearTree";
}


void ServerHandler::rescanTree()
{
    UiLog(this).dbg() << "ServerHandler::rescanTree -->";

	setActivity(RescanActivity);

	//---------------------------------
	// First part of rescan: clearing
	//---------------------------------

	//Stop the timer
	stopRefreshTimer();

	//Stop the queue as a safety measure: we do not want any changes during the rescan
	comQueue_->suspend(false);

	//clear the tree
	clearTree();

	//At this point nothing is running and the tree is empty (it only contains
	//the root node)

	//--------------------------------------
	// Second part of rescan: loading
	//--------------------------------------

	//Create an object to inform the observers about the change
	VServerChange change;

	//Begin the full scan to get the tree. This call does not actually
	//run the scan but counts how many suits will be available.
	vRoot_->beginScan(change);

	//Notify the observers that the scan has started
	broadcast(&ServerObserver::notifyBeginServerScan,change);

	//Finish full scan
	vRoot_->endScan();

	//Notify the observers that scan has ended
    broadcast(&ServerObserver::notifyEndServerScan);

	//Restart the queue
	comQueue_->start();

	//The suites might have been changed
	updateSuiteFilter();

	//Start the timer
	startRefreshTimer();

	setActivity(NoActivity);

    UiLog(this).dbg() << "<-- rescanTree";
}

//====================================================
// Suite filter
//====================================================

void ServerHandler::setSuiteFilterWithOne(VNode* n)
{
    if(n)
    {
        if(VNode* sn=n->suite())
            if(VSuiteNode *suite=sn->isSuite())
            {
                if(suiteFilter_->isEnabled() == false)
                {
                    suiteFilter_->setEnabled(true);
                    suiteFilter_->selectOnlyOne(suite->strName());
                    //after reset the loaded suites need to be read again from the server!
                    suiteFilter_->setLoadedInitialised(false);
                    reset();
                }
                else if(suiteFilter_->isOnlyOneFiltered(suite->strName()) == false)
                {
                    suiteFilter_->selectOnlyOne(suite->strName());
                    //after reset the loaded suites need to be read again from the server!
                    suiteFilter_->setLoadedInitialised(false);
                    reset();
                }
            }
    }
}

void ServerHandler::updateSuiteFilter(SuiteFilter* sf)
{
	if(suiteFilter_->update(sf))
	{
		//If only this flag has changed we exec a custom task for it
		if(suiteFilter_->changeFlags().sameAs(SuiteFilter::AutoAddChanged))
		{
			comQueue_->addSuiteAutoRegisterTask();
		}
		//Otherwise we need a full reset
		else
		{
			reset();
		}

        //Save the settings
        conf_->saveSettings();
	}
}

//Update the suite filter with the list of suites actually loaded onto the server.
//If the suitefilter is enabled this might have only a subset of it in our tree.
void ServerHandler::updateSuiteFilterWithLoaded(const std::vector<std::string>& loadedSuites)
{	
    suiteFilter_->setLoaded(loadedSuites);
}

//Update the suite filter with the list of suites stored in the defs (in the tree). It only
//makes sense if the filter is disabled since in this case the defs stores all the loaded servers.
void ServerHandler::updateSuiteFilterWithDefs()
{
	if(suiteFilter_->isEnabled())
		return;

	std::vector<std::string> defSuites;
	vRoot_->suites(defSuites);
	suiteFilter_->setLoaded(defSuites);
}

//Only called internally after reset or serverscan!!
void ServerHandler::updateSuiteFilter()
{
	//We only fetch the full list of loaded suites from the server
    //via the thread when
    //  -the suiteFilter is not yet initialised
    //   OR
    //  -the suiteFilter is observerved and it is enabled!
    if(!suiteFilter_->isLoadedInitialised() ||
       (suiteFilter_->hasObserver() && suiteFilter_->isEnabled()))
	{
		//This will call updateSuiteFilterWithLoaded()
		comQueue_->addSuiteListTask();
	}
	else
	{
		std::vector<std::string> defSuites;
		vRoot_->suites(defSuites);
		suiteFilter_->setLoaded(defSuites);
	}
}

bool ServerHandler::readFromDisk() const
{
	return conf_->boolValue(VServerSettings::ReadFromDisk);
}

QString ServerHandler::nodeMenuMode() const
{
    return conf_->stringValue(VServerSettings::NodeMenuMode);
}

void ServerHandler::confChanged(VServerSettings::Param par,VProperty* prop)
{
	switch(par)
	{
    case VServerSettings::AutoUpdate:
    case VServerSettings::UpdateRate:
    case VServerSettings::AdaptiveUpdate:
    case VServerSettings::AdaptiveUpdateMode:
    case VServerSettings::AdaptiveUpdateIncrement:
        updateRefreshTimer();
        break;
    case VServerSettings::MaxAdaptiveUpdateRate:
    {
        if(!checkRefreshTimerDrift())
        {
            updateRefreshTimer();
        }
        break;
    }
    case VServerSettings::NodeMenuMode:
        MainWindow::updateMenuMode(this);
        break;

	case VServerSettings::NotifyAbortedEnabled:
	case VServerSettings::NotifyRestartedEnabled:
	case VServerSettings::NotifyLateEnabled:
	case VServerSettings::NotifyZombieEnabled:
	case VServerSettings::NotifyAliasEnabled:
		checkNotificationState(par);
		break;
	default:
		break;
	}
}

void ServerHandler::checkNotificationState(VServerSettings::Param par)
{
	std::string id=VServerSettings::notificationId(par);
	if(id.empty())
		return;

    bool enabled=false;
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
	{
		ServerHandler *s=*it;
		if(s->conf()->boolValue(par))
		{
			enabled=true;
			break;
		}
	}

    ChangeNotify::updateNotificationStateFromServer(id,enabled);
}

//Called from changeNotify
bool ServerHandler::checkNotificationState(const std::string& notifierId)
{
    bool enabled=false;
    VServerSettings::Param par=VServerSettings::notificationParam(notifierId);

    for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
    {
        ServerHandler *s=*it;
        if(s->conf()->boolValue(par))
        {
            enabled=true;
            break;
        }
    }

    return enabled;
}

void ServerHandler::saveSettings()
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
		(*it)->saveConf();
}

void ServerHandler::saveConf()
{
	conf_->saveSettings();
}

void ServerHandler::loadConf()
{
	//This will call confChanged for any non-default settings
	conf_->loadSettings();
}

//--------------------------------------------
// Other
//--------------------------------------------

void ServerHandler::searchBegan()
{
    UiLog(this).dbg() << "ServerHandler::searchBegan --> suspend queue";
	comQueue_->suspend(true);
}

void ServerHandler::searchFinished()
{
    UiLog(this).dbg() << "ServerHandler::searchFinished --> start queue";
	comQueue_->start();

}

int ServerHandler::truncatedLinesFromServer(const std::string& txt) const
{
    //if the text is truncated the following line is added to the bottom of it:
    //# >>>>>>>> File truncated down to 15. Truncated from the end of file <<<<<<<<<
    //We search for this string and if truncation did happen we indicate it in the reply
    size_t txtSize=txt.size();
    if(txt.find(">> File truncated down to",
        (txtSize > 200)?(txtSize-100):0) != std::string::npos)
    {
        return conf_->intValue(VServerSettings::MaxOutputFileLines);
    }

    return -1;
}

//--------------------------------------------------------------
//
//   Find the server for a node.
//   TODO: this is just a backup method. We might not want to use it
//         at all, since it is not safe.
//
//--------------------------------------------------------------

ServerHandler* ServerHandler::find(const std::string& name)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end(); ++it)
			if((*it)->name() == name)
					return *it;
	return NULL;
}
