 //============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ServerHandler.hpp"

#include "Defs.hpp"
#include "ChangeMgrSingleton.hpp"
#include "ClientInvoker.hpp"
#include "File.hpp"
#include "ArgvCreator.hpp"
#include "Str.hpp"

#include "ConnectState.hpp"
#include "NodeObserver.hpp"
#include "ServerObserver.hpp"
#include "UserMessage.hpp"
#include "VNode.hpp"
#include "VTaskObserver.hpp"

#include <QMessageBox>
#include <QMetaType>

#include <iostream>
#include <algorithm>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

std::vector<ServerHandler*> ServerHandler::servers_;
std::map<std::string, std::string> ServerHandler::commands_;

//================================================
//
//                    ServerHandler
//
//================================================

ServerHandler::ServerHandler(const std::string& name,const std::string& host, const std::string& port) :
   name_(name),
   host_(host),
   port_(port),
   client_(0),
   updating_(false),
   communicating_(false),
   comQueue_(0),
   refreshIntervalInSeconds_(60),
   readFromDisk_(true),
   activity_(NoActivity),
   connectState_(new ConnectState())
{
	//Create longname
	longName_=host_ + "@" + port_;

	//Create the client invoker. At this point it is empty.
	client_=new ClientInvoker(host,port);
	//client_->allow_new_client_old_server(1);
	//client_->allow_new_client_old_server(9);
	client_->set_retry_connection_period(1);
	client_->set_throw_on_error(true);

	//Create the vnode root. This will represent the node tree in the viewer, but
	//at this point it is empty.
	vRoot_=new VServer(this);

	//Connect up the timer for refreshing the server info. The timer has not
	//started yet.
	connect(&refreshTimer_, SIGNAL(timeout()),
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

	//We create a ServerComThread here. It is not a member, because we will
	//pass on its ownership to ServerComQueue. At this point the thread is not doing anything.
	ServerComThread* comThread=new ServerComThread(this,client_);

	//The ServerComThread is observing the actual server and its nodes. When there is a change it
	//emits a signal to notify the ServerHandler about it.
	connect(comThread,SIGNAL(nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&)),
					 this,SLOT(slotNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&)));

	connect(comThread,SIGNAL(defsChanged(const std::vector<ecf::Aspect::Type>&)),
						 this,SLOT(slotDefsChanged(const std::vector<ecf::Aspect::Type>&)));

	//Create the queue for the tasks to be sent to the client (via the ServerComThread)! It will
	//take ownership of the ServerComThread. At this point the queue has not started yet.
	comQueue_=new ServerComQueue (this,client_,comThread);

	//-------------------------------------------------
	// At this point nothing is running or active!!!!
	//-------------------------------------------------

	//Indicate that we start an init (initial load)
	activity_=LoadActivity;

	//Try to connect to the server and load the defs etc. This might fail!
	load();

	//We might not have been able to connect to the server. This is indicated by
	//the value of connectedStatus_. Each object needs to be aware of it and
	//do its tasks accordingly.

	//Start the timer
	resetRefreshTimer();
}

ServerHandler::~ServerHandler()
{
	//Notify the observers
	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); it++)
			(*it)->notifyServerDelete(this);

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

	//The safest is to delete the client in the end
	if(client_)
		delete client_;
}

//Completely update the Client Invoker and the graphical tree

void ServerHandler::load()
{
	//This method can only be called when:
	// -the queue is not running
	// -the timer is stopped
	// -the tree is empty.
	//The caller routine must ensure that these conditions are met

	assert(comQueue_->active() == false);
	assert(refreshTimer_.isActive() == false);
	assert(vRoot_->totalNum() == 0);
	assert(vRoot_->numOfChildren() == 0);

	//The server must be disconnected or we have to be in a reset to continue
	//if(activity_!= ResetActivity && connectState_ == Normal)
	//	return;

	//There are no observers at this point
	//Notify the observers that the init has begun
	//notifyServerObservers(&ServerObserver::notifyServerInitBegin);

	//Clear the connect error text message
	//connectError_.clear();

	//Indicate that we start an init (initial load)
	activity_=LoadActivity;

	//Try to get the server version via the client. If it is successful
	//we can be sure that we can connect to the server.
	/*try
	{
		//Get the server version
		std::string server_version;
		client_->server_version();
		server_version = client_->server_reply().get_string();

		UserMessage::message(UserMessage::DBG, false,
			       std::string("ecflow server version: ") + server_version);

		//if (!server_version.empty()) return;
	}

	//The init failed
	catch(std::exception& e)
	{
		connectState_->errorMessage(e.what());
		UserMessage::message(UserMessage::DBG, false, std::string("failed to sync: ") + e.what());

		//The init has failed
		loadFailed();
		return;
	}*/

	//If we are here we can be sure that we can connect to the server and get the defs. Instruct the queue
	//to run the init task. It will eventually call initEnd() with the right argument!!

	//NOTE: at this point the queue is not running but load will start it.
	//While the queue is in load mode it does not accept tasks.
	comQueue_->load();
}

//The load was sussessfull
void ServerHandler::loadFinished()
{
	activity_=NoActivity;

	//Set the connection state
	connectState_->state(ConnectState::Normal);
	broadcast(&ServerObserver::notifyServerConnectState);

	//Set server host and port in defs. It is used to find the server of
	//a given node in the viewer.
	{
		ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction

		defs_ptr defs = defsAccess.defs();
		if(defs != NULL)
		{
			ServerState& st=defs->set_server();
			st.hostPort(std::make_pair(host_,port_));
			st.add_or_update_user_variables("nameInViewer",name_);
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

	//Notify the observers that scan has ended
	broadcast(&ServerObserver::notifyEndServerScan);
}

//The load failed and we could not connect to the server, e.g. because the the server
//may be down, or there is a network error, or the authorisation is missing.
void ServerHandler::loadFailed(const std::string& errMsg)
{
	//This status is indicated by the connectStat_. Each object needs to be aware of it
	//and do its tasks accordingly.

	connectState_->state(ConnectState::Lost);
	connectState_->errorMessage(errMsg);
	activity_=NoActivity;

	broadcast(&ServerObserver::notifyServerConnectState);
}

void ServerHandler::stopRefreshTimer()
{
	refreshTimer_.stop();
}

void ServerHandler::resetRefreshTimer()
{
	//If we are not connected to the server the
	//timer should not run.
	if(connectState_->state() == ConnectState::Disconnected)
		return;

	// interval of -1 means don't use a timer
	if (refreshIntervalInSeconds_ != -1)
	{
		refreshTimer_.stop();
		refreshTimer_.setInterval(refreshIntervalInSeconds_*1000);
		refreshTimer_.start();
	}
	else
	{
		refreshTimer_.stop();
	}
}


ServerHandler* ServerHandler::addServer(const std::string& name,const std::string& host, const std::string& port)
{
	ServerHandler* sh=new ServerHandler(name,host,port);
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

SState::State ServerHandler::serverState()
{
	if(connectState_->state() != ConnectState::Normal || activity_== LoadActivity)
		return SState::RUNNING;

	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if(defs != NULL)
	{
		ServerState& st=defs->set_server();
		return st.get_state();
	}
	return SState::RUNNING;
}

NState::State ServerHandler::state(bool& suspended)
{
	if(connectState_->state() != ConnectState::Normal || activity_== LoadActivity)
		return NState::UNKNOWN;

	suspended=false;
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if(defs != NULL)
	{
		suspended=defs->isSuspended();
		return defs->state();
	}
	return NState::UNKNOWN;
}

defs_ptr ServerHandler::defs()
{
	if(client_)
	{
		return client_->defs();
	}
	else
	{
		defs_ptr null;
		return null;
	}
}

void ServerHandler::releaseDefs()
{
	defsMutex_.unlock();   // unlock addess to the defs for this thread
}



void ServerHandler::errorMessage(std::string message)
{
	UserMessage::message(UserMessage::ERROR, true, message);
}

//-------------------------------------------------------------
//
// Query for node information. It might use the client invoker's
// threaded communication!
//
//--------------------------------------------------------------

//The preferred way to run client commands is to add a command task to the queue. The
//queue will manage the task and send it to the ClientInvoker. When the task
//finishes the ServerHandler::clientTaskFinished method is called where the
//result/reply can be processed.

void ServerHandler::runCommand(const std::vector<std::string>& cmd)
{
	if(connectState_->state() == ConnectState::Disconnected)
		return;

	VTask_ptr task=VTask::create(VTask::CommandTask);
	task->command(cmd);
	comQueue_->addTask(task);
}


//The preferred way to run client tasks is to define and add a task to the queue. The
//queue will manage the task and will send it to the ClientInvoker. When the task
//finishes the ServerHandler::clientTaskFinished method is called where the
//result/reply can be processed.

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
	case VTask::MessageTask:
	case VTask::StatsTask:
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
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	task->param("clientPar","script");
	comQueue_->addTask(task);
}

void ServerHandler::job(VTask_ptr task)
{
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	task->param("clientPar","job");
	comQueue_->addTask(task);
}

void ServerHandler::jobout(VTask_ptr task)
{
	static std::string errText="no job output...";

	task->param("clientPar","jobout");
	comQueue_->addTask(task);
}

void ServerHandler::manual(VTask_ptr task)
{
	std::string errText="no manual ...";
	task->param("clientPar","manual");
	comQueue_->addTask(task);
}


void ServerHandler::updateAll()
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
	{
		(*it)->update();
		(*it)->resetRefreshTimer();  // to avoid too many server requests
	}
}

// see view/host.cc / ehost::update() for full code
int ServerHandler::update()
{
	//We add and update task to the queue. On startup this function can be called
	//before the comQueue_ was created so we need to check if it exists.
	if(comQueue_ && comQueue_->active())
	{
		comQueue_->addNewsTask();
	}

	return 0;
}

//This slot is called by the timer regularly to get news from the server.
void ServerHandler::refreshServerInfo()
{
	UserMessage::message(UserMessage::DBG, false, std::string("auto refreshing server info for ") + name());
	update();
}

std::string ServerHandler::commandToString(const std::vector<std::string>& cmd)
{
	return boost::algorithm::join(cmd," ");
}

//Send a command to a server. The command is specified as a string vector, while the node or server for that
//the command will be applied is specified in a VInfo object.
void ServerHandler::command(VInfo_ptr info,const std::vector<std::string>& cmd, bool resolve)
{
	std::vector<std::string> realCommand=cmd;

	if (!realCommand.empty())
	{
		UserMessage::message(UserMessage::DBG, false, std::string("command: ") + commandToString(realCommand));

		//Get the name of the object for that the command will be applied
		std::string nodeFullName;
		std::string nodeName;
		ServerHandler* serverHandler = info->server();

		if(info->isNode())
		{
			nodeFullName = info->node()->node()->absNodePath();
			nodeName = info->node()->node()->name();
			//UserMessage::message(UserMessage::DBG, false, std::string("  --> for node: ") + nodeFullName + " (server: " + info[i]->server()->longName() + ")");
		}
		else if(info->isServer())
		{
			nodeFullName = "/";
			nodeName = "/";
			//UserMessage::message(UserMessage::DBG, false, std::string("  --> for server: ") + nodeFullName);
		}

		//Replace placeholders with real node names
		for(unsigned int i=0; i < cmd.size(); i++)
		{
			if(realCommand[i]=="<full_name>")
				realCommand[i]=nodeFullName;
			else if(realCommand[i]=="<node_name>")
				realCommand[i]=nodeName;
		}

		UserMessage::message(UserMessage::DBG, false, std::string("final command: ") + commandToString(realCommand));

		// get the command into the right format by first splitting into tokens
		// and then converting to argc, argv format

		//std::vector<std::string> strs;
		//std::string delimiters(" ");
		//ecf::Str::split(realCommand, strs, delimiters);

		// set up and run the thread for server communication
		serverHandler->runCommand(realCommand);
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("command is not recognised."));
	}
}
//Send the same command for a list of objects (nodes/servers) specified in a VInfo vector.
//The command is specified as a string.

void ServerHandler::command(std::vector<VInfo_ptr> info, std::string cmd, bool resolve)
{
	std::string realCommand;

	// is this a shortcut name for a command, or the actual command itself?
	if (resolve)
		realCommand = resolveServerCommand(cmd);
	else
		realCommand = cmd;

	std::vector<ServerHandler *> targetServers;

	if (!realCommand.empty())
	{
		UserMessage::message(UserMessage::DBG, false, std::string("command: ") + cmd + " (real: " + realCommand + ")");

		std::map<ServerHandler*,std::string> targetNodeNames;
		std::map<ServerHandler*,std::string> targetNodeFullNames;

		//Figure out what objects (node/server) the command should be applied to
		for(int i=0; i < info.size(); i++)
		{
			std::string nodeFullName;
			std::string nodeName = info[i]->node()->node()->name();

			//Get the name
			if(info[i]->isNode())
			{
				nodeFullName = info[i]->node()->node()->absNodePath();
				//UserMessage::message(UserMessage::DBG, false, std::string("  --> for node: ") + nodeFullName + " (server: " + info[i]->server()->longName() + ")");
			}
			else if(info[i]->isServer())
			{
				nodeFullName = info[i]->server()->longName();
				//UserMessage::message(UserMessage::DBG, false, std::string("  --> for server: ") + nodeFullName);
			}

			//Storre the names per target servers
			targetNodeNames[info[i]->server()] += " " + nodeName;
			targetNodeFullNames[info[i]->server()] += " " + nodeFullName;

			//info[i]->server()->targetNodeNames_     += " " + nodeName;      // build up the list of nodes for each server
			//info[i]->server()->targetNodeFullNames_ += " " + nodeFullName;  // build up the list of nodes for each server


			// add this to our list of target servers?
			if (std::find(targetServers.begin(), targetServers.end(), info[i]->server()) == targetServers.end())
			{
				targetServers.push_back(info[i]->server());
			}
		}


		// for each target server, construct and send its command

		for (size_t s = 0; s < targetServers.size(); s++)
		{
			ServerHandler* serverHandler = targetServers[s];

			// replace placeholders with real node names

			std::string placeholder("<full_name>");
			//ecf::Str::replace_all(realCommand, placeholder, serverHandler->targetNodeFullNames_);
			ecf::Str::replace_all(realCommand, placeholder, targetNodeFullNames[serverHandler]);

			placeholder = "<node_name>";
			//ecf::Str::replace_all(realCommand, placeholder, serverHandler->targetNodeNames_);
			ecf::Str::replace_all(realCommand, placeholder, targetNodeNames[serverHandler]);

			UserMessage::message(UserMessage::DBG, false, std::string("final command: ") + realCommand);


			// get the command into the right format by first splitting into tokens
			// and then converting to argc, argv format

			std::vector<std::string> strs;
			std::string delimiters(" ");
			ecf::Str::split(realCommand, strs, delimiters);

			// set up and run the thread for server communication
			serverHandler->runCommand(strs);

			//serverHandler->targetNodeNames_.clear();      // reset the target node names for next time
			//serverHandler->targetNodeFullNames_.clear();  // reset the target node names for next time
			//serverHandler->update();
		}
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("command ") + cmd + " is not recognised. Check the menu definition.");
	}
}

void ServerHandler::addServerCommand(const std::string &name, const std::string command)
{
	commands_[name] = command;
}

std::string ServerHandler::resolveServerCommand(const std::string &name)
{
	std::string realCommand;

	// is this command registered?
	std::map<std::string,std::string>::iterator it = commands_.find(name);

	if (it != commands_.end())
	{
		realCommand = it->second;
	}
	else
	{
		realCommand = "";
		UserMessage::message(UserMessage::WARN, true, std::string("Command: ") + name + " is not registered" );
	}

	return realCommand;
}


//======================================================================================
// Manages node changes and node observers. Node observers are notified when
// any of the nodes changes.
//======================================================================================

//This slot is called when a node changes.
void ServerHandler::slotNodeChanged(const Node* nc, const std::vector<ecf::Aspect::Type>& aspect)
{
	VNode* vn=vRoot_->toVNode(nc);

	//We must have this VNode
	assert(vn != NULL);

	//Create an object adding some more details about the change
	VNodeChange change;

	//Begin update for the VNode
	vRoot_->beginUpdate(vn,aspect,change);

	//If the node has not been used by any of the views so far we ignore the update
	//TODO: what about the infopanel or breadcrumbs??????
	if(change.ignore_)
	{
		return;
	}
	//If too many things changed we simply reset
	else if(change.reset_)
	{
		/*
		//Notify the observers
		broadcast(&NodeObserver::notifyBeginNodeClear,vn);

		//End update for the VNode
		vRoot_->clear(vn);

		//Notify the observers
		broadcast(&NodeObserver::notifyEndNodeClear,vn);

		//Notify the observers
		broadcast(&NodeObserver::notifyBeginNodeScan,vn);

		//End update for the VNode
		vRoot_->beginScan();

		//Notify the observers
		broadcast(&NodeObserver::notifyEndNodeScan,vn);*/

		reset();
	}
	//Otherwise continue with the update
	else
	{
		//Notify the observers
		broadcast(&NodeObserver::notifyBeginNodeChange,vn,aspect,change);

		//End update for the VNode
		vRoot_->endUpdate(vn,aspect);

		//Notify the observers
		broadcast(&NodeObserver::notifyEndNodeChange,vn,aspect,change);
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
	for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); it++)
		((*it)->*proc)(node);
}

void ServerHandler::broadcast(NoMethodV1 proc,const VNode* node,const std::vector<ecf::Aspect::Type>& aspect,const VNodeChange& change)
{
	for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); it++)
		((*it)->*proc)(node,aspect,change);
}

//---------------------------------------------------------------------------
// Manages Defs changes and desf observers. Defs observers are notified when
// there is a change.
//---------------------------------------------------------------------------

//This slot is called when the Defs change.
void ServerHandler::slotDefsChanged(const std::vector<ecf::Aspect::Type>& a)
{
	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); it++)
		(*it)->notifyDefsChanged(this,a);
}

void ServerHandler::addServerObserver(ServerObserver *obs)
{
	std::vector<ServerObserver*>::iterator it=std::find(serverObservers_.begin(),serverObservers_.end(),obs);
	if(it == serverObservers_.end())
	{
		serverObservers_.push_back(obs);
	}
}

void ServerHandler::removeServerObserver(ServerObserver *obs)
{
	std::vector<ServerObserver*>::iterator it=std::find(serverObservers_.begin(),serverObservers_.end(),obs);
	if(it != serverObservers_.end())
	{
		serverObservers_.erase(it);
	}
}

void ServerHandler::broadcast(SoMethod proc)
{
	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); it++)
		((*it)->*proc)(this);
}

void ServerHandler::broadcast(SoMethodV1 proc,const VServerChange& ch)
{
	for(std::vector<ServerObserver*>::const_iterator it=serverObservers_.begin(); it != serverObservers_.end(); it++)
		((*it)->*proc)(this,ch);
}

//-------------------------------------------------------------------
// This slot is called when the comThread finished the given task!!
//-------------------------------------------------------------------

void ServerHandler::clientTaskFinished(VTask_ptr task,const ServerReply& serverReply)
{
	UserMessage::message(UserMessage::DBG, false, std::string("ServerHandler::commandSent"));

	//See which type of task finished. What we do now will depend on that.
	switch(task->type())
	{
		case VTask::CommandTask:
		{
			// a command was sent - we should now check whether there have been
			// any updates on the server (there should have been, because we
			// just did something!)

			UserMessage::message(UserMessage::DBG, false, std::string("Send command to server"));
			comQueue_->addNewsTask();
			break;
		}
		case VTask::NewsTask:
		{
			// we've just asked the server if anything has changed - has it?

			switch (serverReply.get_news())
			{
				case ServerReply::NO_NEWS:
				{
					// no news, nothing to do
					UserMessage::message(UserMessage::DBG, false, std::string("No news from server"));
					connectionGained();
					break;
				}

				case ServerReply::NEWS:
				{
					// yes, something's changed - synchronise with the server

					UserMessage::message(UserMessage::DBG, false, std::string("News from server - send sync command"));
					connectionGained();
					comQueue_->addSyncTask(); //comThread()->sendCommand(this, client_, ServerComThread::SYNC);
					break;
				}

				case ServerReply::DO_FULL_SYNC:
				{
					// yes, a lot of things have changed - we need to reset!!!!!!

					UserMessage::message(UserMessage::DBG, false, std::string("Do_FULL_SYNC from server"));
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
		case VTask::LoadTask:
		{
			//If not yet connected but the sync task was successful
			loadFinished();
			break;
		}

		case VTask::ScriptTask:
		case VTask::ManualTask:
		{
			task->reply()->text(serverReply.get_string());
			task->status(VTask::FINISHED);
			break;
		}

		case VTask::MessageTask:
		{
			task->reply()->text(serverReply.get_string_vec());
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

		default:
			break;

	}
}

//-------------------------------------------------------------------
// This slot is called when the comThread finished the given task!!
//-------------------------------------------------------------------

void ServerHandler::clientTaskFailed(VTask_ptr task,const std::string& errMsg)
{
	//UserMessage::message(UserMessage::DBG, false, std::string("ServerHandler::commandSent"));

	//See which type of task finished. What we do now will depend on that.
	switch(task->type())
	{
		//The initialisation failed
		case VTask::LoadTask:
		{
			loadFailed(errMsg);
			break;
		}
		case VTask::NewsTask:
		case VTask::StatsTask:
		{
			connectionLost(errMsg);
			break;
		}
		default:
			task->reply()->errorText(errMsg);
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

void ServerHandler::connectionGained()
{
	if(connectState_->state() != ConnectState::Normal)
	{
		connectState_->state(ConnectState::Normal);
		broadcast(&ServerObserver::notifyServerConnectState);
	}
}

void ServerHandler::disconnectServer()
{
	if(connectState_->state() != ConnectState::Disconnected)
	{
		connectState_->state(ConnectState::Disconnected);
		broadcast(&ServerObserver::notifyServerConnectState);

		//Stop the queue
		comQueue_->stop();

		//Stop the timer
		stopRefreshTimer();
	}
}

void ServerHandler::connectServer()
{
	if(connectState_->state() == ConnectState::Disconnected)
	{
		//Start the queue
		comQueue_->start();

		//Start the timer
		resetRefreshTimer();

		//Try to get the news
		update();
	}
}

//It is just for testing
void ServerHandler::resetFirst()
{
	if(servers_.size() > 0)
		servers_.at(0)->reset();
}

void ServerHandler::reset()
{
	//---------------------------------
	// First part of reset: clearing
	//---------------------------------

	//Stop the timer
	stopRefreshTimer();

	//Empty and stop the queue
	comQueue_->stop();

	//Notify observers that the clear is about to begin
	broadcast(&ServerObserver::notifyBeginServerClear);

	//Clear vnode
	vRoot_->clear();

	//Reset client handle and defs as well. This does not require
	//communications with the server.
	client_->reset();

	//Notify observers that the clear ended
	broadcast(&ServerObserver::notifyEndServerClear);

	//At this point nothing is running and the tree is empty (it only contains
	//the root node)

	//--------------------------------------
	// Second part of reset: loading
	//--------------------------------------

	//We simply call load again
	load();
}


/*
void ServerHandler::commandSent(){
	UserMessage::message(UserMessage::DBG, false, std::string("ServerHandler::commandSent"));

	// which type of command was sent? What we do now will depend on that.

	switch (comThread()->commandType())
	{
		case ServerComThread::COMMAND:
		{
			// a command was sent - we should now check whether there have been
			// any updates on the server (there should have been, because we
			// just did something!)

			UserMessage::message(UserMessage::DBG, false, std::string("Send command to server"));
			comThread()->sendCommand(this, client_, ServerComThread::NEWS);
			break;
		}

		case  ServerComThread::NEWS:
		{
			// we've just asked the server if anything has changed - has it?

			switch (client_->server_reply().get_news())
			{
				case ServerReply::NO_NEWS:
				{
					// no news, nothing to do
					UserMessage::message(UserMessage::DBG, false, std::string("No news from server"));
					setUpdatingStatus(false);  // finished updating
					break;
				}

				case ServerReply::NEWS:
				{
					// yes, something's changed - synchronise with the server

					UserMessage::message(UserMessage::DBG, false, std::string("News from server - send sync command"));
					comThread()->sendCommand(this, client_, ServerComThread::SYNC);
					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}
		
		case ServerComThread::SYNC:
		{
			// yes, something's changed - synchronise with the server

			UserMessage::message(UserMessage::DBG, false, std::string("We've synced"));
			setUpdatingStatus(false);  // finished updating
			break;
		}

		default:
		{
			break;
		}

	}

}
*/

//--------------------------------------------------------------
//
//   Find the server for a node.
//   TODO: this is just a backup method. We might not want to use it
//         at all, since it is not safe.
//
//--------------------------------------------------------------

ServerHandler* ServerHandler::find(const std::string& name)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
			if((*it)->name() == name)
					return *it;
	return NULL;
}

ServerHandler* ServerHandler::find(Node *node)
{
	// XXXXX here we should protect access to the definitions, but we
	// don't yet know which defs we are accessing!

	if(node)
	{
		if(Defs* defs = node->defs())
		{
			const ServerState& st=defs->server();

			//It is a question if this solution is fast enough. We may need to store
			//directly the server name in ServerState
			st.find_variable("nameInViewer");

			return ServerHandler::find(st.find_variable("nameInViewer"));
		}
	}
	return NULL;
}

//===============================================================================
//
//                         ServerComQueue
//
//===============================================================================

// This class manages the tasks to be sent to the ServerComThread, which controls
// the communication with the ClientInvoker. The ClientInvoker is hidden from the
// ServerHandler. The ServerHandler needs to define a task and send it to
// ServerComQueue then it will pass it on to the ClientInvoker. When the task is finished
// ServerComQueue notifies the ServerHandler about it.

ServerComQueue::ServerComQueue(ServerHandler *server,ClientInvoker *client, ServerComThread *comThread) :
	QObject(server),
	server_(server),
	client_(client),
	comThread_(comThread),
	wait_(false),
	active_(false),
	load_(false)
{
	timer_=new QTimer(this);
	timer_->setInterval(2000);

	connect(timer_,SIGNAL(timeout()),
			this,SLOT(slotRun()));

	//When the ServerComThread finishes its task it emits a signal that
	//is connected to the queue.
	connect(comThread_, SIGNAL(finished()),
			this, SLOT(slotTaskFinished()));

	//When there is an error in the ServerComThread task emits the
	//failed() signal that is connected to the queue.
	connect(comThread_, SIGNAL(failed(std::string)),
			this, SLOT(slotTaskFailed(std::string)));
}

ServerComQueue::~ServerComQueue()
{
	//Stop the queue
	stop();

	//Disconnects all the signals from the thread
	comThread_->disconnect(0,this);

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	delete comThread_;
}

//This is a special mode to reload the whole ClientInvoker
void ServerComQueue::load()
{
	if(load_)
		return;

	//Set the load status
	load_=true;

	//The queue must be stopped
	stop();

	//The thread cannot be running
	assert(comThread_->isRunning() == false);

	//We send and init command straight away to the thread!!
	VTask_ptr task=VTask::create(VTask::LoadTask);
	current_=task;
	comThread_->task(current_);

	//The queue is still stopped!!!
}

void ServerComQueue::endLoad()
{
	if(load_)
	{
		//Set the load status
		load_=false;

		//We restart the queue
		start();
	}
}


//When the queue is started:
// -it is ready to accept tasks
// -its timer is running
void ServerComQueue::start()
{
	if(active_==true)
		return;

	//Set the status
	active_=true;

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//comThread_->attach();

	//Starts the timer
	timer_->start();
}

//When the queue is stopped:
// -it is empty
// -it does not accept tasks
// -its timer is stopped
// -the thread is detached from ClientInvoker
void ServerComQueue::stop()
{
	if(active_==false)
		return;

	//Set the status
	active_=false;

	//Empty the tasks
	tasks_.clear();

	//Stop the timer
	timer_->stop();

	//If the comthread is running we need to wait
	//until it finishes its task.
	comThread_->wait();

	//Detach the thread from the ClientInvoker
	comThread_->detach();

	//Clear the current task
	if(current_)
		current_.reset();
}

void ServerComQueue::addTask(VTask_ptr task)
{
	if(!task)
		return;

	if(!active_ || (task && task->type() ==VTask::LoadTask) )
		return;

	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::addNewsTask()
{
	if(!active_)
		return;

	VTask_ptr task=VTask::create(VTask::NewsTask);
	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::addSyncTask()
{
	if(!active_)
		return;

	VTask_ptr task=VTask::create(VTask::SyncTask);
	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::slotRun()
{
	if(!active_)
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
	//We need to leave the load mode
	endLoad();

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
	//We need to leave the load mode
	endLoad();

	//We notify the server that the task has failed
	server_->clientTaskFailed(current_,msg);

	//current_->reply()->errorText(msg);
	//current_->status(VTask::ABORTED);

	//We do not need the current task any longer.
	if(current_)
		current_.reset();
}

//==============================================================
//
//                     ServerComThread
//
//==============================================================

ServerComThread::ServerComThread(ServerHandler *server, ClientInvoker *ci) :
		server_(server),
		ci_(ci),
		taskType_(VTask::NoTask),
		attached_(false)
{
}

ServerComThread::~ServerComThread()
{
	detach();
}

void ServerComThread::task(VTask_ptr task)
{
	// do not execute thread if already running

	if (isRunning())
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("ServerComThread::sendCommand - thread already running, will not execute command"));
	}
	else
	{
		//if(!server_ && server)
		//	initObserver(server);

		//We set the parameters needed to run the task. These members are not protected by
		//a mutex, because apart from this task() function only run() can access them!!
		command_=task->command();
		params_=task->params();
		nodePath_.clear();
		taskType_=task->type();
		nodePath_=task->targetPath();

		//Start the thread execution
		start();
	}
}

void ServerComThread::run()
{
	//Can we use it? We are in the thread!!!
	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run start"));

	UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run start path: ") + nodePath_);

	try
 	{
		switch (taskType_)
		{
			case VTask::CommandTask:
			{
				// call the client invoker with the saved command
				UserMessage::message(UserMessage::DBG, false, std::string("    COMMAND"));
				ArgvCreator argvCreator(command_);
				//UserMessage::message(UserMessage::DBG, false, argvCreator.toString());
				ci_->invoke(argvCreator.argc(), argvCreator.argv());
				break;
			}

			case VTask::NewsTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    NEWS"));
				ci_->news_local(); // call the server
				break;
			}

			case VTask::SyncTask:
			{
				ServerDefsAccess defsAccess(server_);
				UserMessage::message(UserMessage::DBG, false, std::string("    SYNC"));
				ci_->sync_local();
				UserMessage::message(UserMessage::DBG, false, std::string("    SYNC FINISHED"));
				break;
			}

			case VTask::LoadTask:
			{
				{
					//sleep(15);
					ServerDefsAccess defsAccess(server_);
					UserMessage::message(UserMessage::DBG, false, std::string("    INIT SYNC"));
					ci_->sync_local();
					UserMessage::message(UserMessage::DBG, false, std::string("    INIT SYNC FINISHED"));
				}

				//Attach the observers to the server
				attach();
				break;
			}

			case VTask::JobTask:
			case VTask::ManualTask:
			case VTask::ScriptTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    FILE"));
				ci_->file(nodePath_,params_["clientPar"]);
				break;
			}

			case VTask::MessageTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    HISTORY"));
				ci_->edit_history(nodePath_);
				break;
			}

			case VTask::StatsTask:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    STATS"));
				ci_->stats();
				break;
			}

			default:
			{

			}
		}
	}

	catch(std::exception& e)
	{
		// note that we need to emit a signal rather than directly call a message function
		// because we can't call Qt widgets from a worker thread

		std::string errorString = e.what();
		Q_EMIT failed(errorString);

		UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run failed: ") + errorString);

		//This will stop the thread.
		return;
	}

	//Can we use it? We are in the thread!!!
	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run finished"));
}

void ServerComThread::update(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	UserMessage::message(UserMessage::DBG, false, std::string("Thread update - node: ") + node->name());
	for(std::vector<ecf::Aspect::Type>::const_iterator it=types.begin(); it != types.end(); it++)
		UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + boost::lexical_cast<std::string>(*it));

	Q_EMIT nodeChanged(node,types);
}


void ServerComThread::update(const Defs* dc, const std::vector<ecf::Aspect::Type>& types)
{
	UserMessage::message(UserMessage::DBG, false, std::string("Thread update - defs: "));
	for(std::vector<ecf::Aspect::Type>::const_iterator it=types.begin(); it != types.end(); it++)
			UserMessage::message(UserMessage::DBG, false, std::string(" aspect: ") + boost::lexical_cast<std::string>(*it));

	Q_EMIT defsChanged(types);
}

void ServerComThread::update_delete(const Node* nc)
{
	Node *n=const_cast<Node*>(nc);
	ChangeMgrSingleton::instance()->detach(n,this);
}

void ServerComThread::update_delete(const Defs* dc)
{
	Defs *d=const_cast<Defs*>(dc);
	ChangeMgrSingleton::instance()->detach(static_cast<Defs*>(d),this);
}

//Register each node to the observer
void ServerComThread::attach()
{
	if(attached_)
		return;

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	int cnt=0;

	ChangeMgrSingleton::instance()->attach(d.get(),this);

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		attach(suites.at(i).get());
	}

	attached_=true;
}

//Add a node to the observer
void ServerComThread::attach(Node *node)
{
	ChangeMgrSingleton::instance()->attach(node,this);

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		attach((*it).get());
	}
}

//Remove each node from the observer
void ServerComThread::detach()
{
	if(!attached_)
		return;

	ServerDefsAccess defsAccess(server_);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	int cnt=0;

	ChangeMgrSingleton::instance()->detach(d.get(),this);

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		detach(suites.at(i).get());
	}

	attached_=false;
}

//Remove each node from the observer
void ServerComThread::detach(Node *node)
{
	ChangeMgrSingleton::instance()->detach(node,this);

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);

	for(std::vector<node_ptr>::const_iterator it=nodes.begin(); it != nodes.end(); it++)
	{
		detach((*it).get());
	}
}

// ------------------------------------------------------------
//                         ServerDefsAccess
// ------------------------------------------------------------


ServerDefsAccess::ServerDefsAccess(ServerHandler *server) :
	server_(server)
{
	server_->defsMutex_.lock();  // lock the resource on construction
}


ServerDefsAccess::~ServerDefsAccess()
{
	server_->defsMutex_.unlock();  // unlock the resource on destruction
}


defs_ptr ServerDefsAccess::defs()
{
	return server_->defs();		// the resource will always be locked when we use it
}
