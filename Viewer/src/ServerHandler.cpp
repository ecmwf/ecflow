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
#include "MainWindow.hpp"
#include "NodeObserver.hpp"
#include "ServerObserver.hpp"
#include "UserMessage.hpp"
#include "VTaskObserver.hpp"

#include <QMessageBox>

#include <iostream>
#include <algorithm>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

std::vector<ServerHandler*> ServerHandler::servers_;
std::map<std::string, std::string> ServerHandler::commands_;

//================================================
//
// ServerHandler
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
   refreshIntervalInSeconds_(10)
{
	longName_=host_ + "@" + port_;

	client_=new ClientInvoker(host,port);
	//client_->allow_new_client_old_server(1);
	//client_->allow_new_client_old_server(9);

	std::string server_version;
	client_->server_version();
	server_version = client_->server_reply().get_string();
	UserMessage::message(UserMessage::DBG, false, std::string("ecflow server version: ") + server_version);

	client_->sync_local();

	//Set server host and port in defs
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


	// we'll need to pass std::strings via signals and slots for error messages
	if (servers_.empty())
	{
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<QList<ecf::Aspect::Type> >("QList<ecf::Aspect::Type>");
		qRegisterMetaType<std::vector<ecf::Aspect::Type> >("std::vector<ecf::Aspect::Type>");
	}

	servers_.push_back(this);

	// populate the map of server commands - in the future this will be done when
	// the configuration file is parsed
	//if (commands_.empty())
	//{
	//	addServerCommand("Requeue", "ecflow_client --requeue force <full_name>");
	//	addServerCommand("Execute", "ecflow_client --run <full_name>");
	//}


	// XXX we may not always want to create a thread here because of resource
	// issues; another strategy would be to create threads on demand, only
	// when server communication is about to start

	//We create a ServerComThread here. It is not a member, because we will
	//pass on its ownership to ServerComQueue.
	ServerComThread* comThread=new ServerComThread(this,client_);

	//The ServerComThread is observing the actual server and its nodes. When there is a change it
	//emits a signal a notifies the ServerHandler about it.
	connect(comThread,SIGNAL(nodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&)),
				 this,SLOT(slotNodeChanged(const Node*, const std::vector<ecf::Aspect::Type>&)));

	connect(comThread,SIGNAL(defsChanged(const std::vector<ecf::Aspect::Type>&)),
					 this,SLOT(slotDefsChanged(const std::vector<ecf::Aspect::Type>&)));

	//connect(comThread(), SIGNAL(errorMessage(std::string)),
	//		this, SLOT(errorMessage(std::string)));

	//Create queue for the tasks to be sent to the client (via the ServerComThread)! It will
	//take ownership of the ServerComThread.
	comQueue_=new ServerComQueue (this,client_,comThread);

	// set the timer for refreshing the server info
   	connect(&refreshTimer_, SIGNAL(timeout()), this, SLOT(refreshServerInfo()));
	resetRefreshTimer();
}

ServerHandler::~ServerHandler()
{
	if(client_)
		delete client_;

	if (comQueue_)
			delete comQueue_;
}


void ServerHandler::resetRefreshTimer()
{
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

SState::State ServerHandler::state()
{
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr defs = defsAccess.defs();
	if(defs != NULL)
	{
			ServerState& st=defs->set_server();
			return st.get_state();
	}
	return SState::RUNNING;
}


int ServerHandler::numSuites()
{
	if(client_)
	{
		ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if(defs != NULL)
		{
			return static_cast<int>(defs->suiteVec().size());
		}
	}
	return 0;
}

node_ptr ServerHandler::suiteAt(int pos)
{
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d != NULL)
	{
		const std::vector<suite_ptr> &suites = d->suiteVec();
		return (pos >=0 && pos < suites.size())?suites.at(pos):node_ptr();
	}

	return node_ptr();
}

int ServerHandler::indexOfSuite(Node* node)
{
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d != NULL)
	{
			const std::vector<suite_ptr> &suites = d->suiteVec();
			for(unsigned int i=0; i < suites.size(); i++)
					if(suites.at(i).get() == node)
							return i;
		}

	return -1;


}

int ServerHandler::numberOfNodes()
{
	int num=0;
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d != NULL)
	{
			const std::vector<suite_ptr> &suites = d->suiteVec();
			for(unsigned int i=0; i < suites.size(); i++)
			{
				num+=1;
				std::set<Node*> n;
				suites.at(i)->allChildren(n);
				num+=static_cast<int>(n.size());
			}
		}

	return num;
}

Node* ServerHandler::findNode(int globalIndex)
{
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr d=defsAccess.defs();
	if(d == NULL)
			return 0;

	int total=0;
	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
			if(total == globalIndex)
					return suites.at(i).get();

			total+=1;

			Node *n=findNode(globalIndex,total,suites.at(i).get());
			if(n)
				return n;
	}

	return 0;
}

Node* ServerHandler::findNode(int globalIndex,int& total,Node *parent)
{
	std::vector<node_ptr> nodes;
	parent->immediateChildren(nodes);
	if(globalIndex < total+nodes.size())
	{
		int pos=globalIndex-total;
		return nodes.at(pos).get();
	}
	else
	{
		total+=nodes.size();
		for(unsigned int i=0; i < nodes.size();i++)
		{
			Node *n=findNode(globalIndex,total,nodes.at(i).get());
			if(n)
				return n;
		}
	}

	return 0;
}



Node* ServerHandler::immediateChildAt(Node *parent,int pos)
{
	if(!parent || pos <0) return NULL;

	std::vector<node_ptr> nodes;
	parent->immediateChildren(nodes);
	if(static_cast<int>(nodes.size()) > pos)
		return nodes.at(pos).get();

	return NULL;
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


int ServerHandler::numOfImmediateChildren(Node *node)
{
	if(!node) return 0;

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);
	return static_cast<int>(nodes.size());
}

int ServerHandler::indexOfImmediateChild(Node *node)
{
	if(!node) return 0;

	std::vector<node_ptr> nodes;
	if(node->parent())
	{
			node->parent()->immediateChildren(nodes);
			for(unsigned int i=0; i < nodes.size(); i++)
				if(nodes.at(i).get() == node)
						return i;
	}

	return -1;
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
		return messages(task);
		break;
	case VTask::StatsTask:
		return stats(task);
		break;
	default:
		break;
	}

	//If we are here we have an unhandled task type.
	//task->cancel("ServerHandler cannot handle this type of task!");
	task->status(VTask::REJECTED);
}

void ServerHandler::messages(VTask_ptr task)
{
	comQueue_->addTask(task);
}

void ServerHandler::script(VTask_ptr req)
{
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	req->param("ecfVar","ECF_SCRIPT");
	req->param("clientPar","script");

	file(req,errText);
}

void ServerHandler::job(VTask_ptr req)
{
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	req->param("ecfVar","ECF_JOB");
	req->param("clientPar","job");

	file(req,errText);
}

void ServerHandler::jobout(VTask_ptr req)
{
	static std::string errText="no job output...";

	req->param("ecfVar","ECF_JOBOUT");
	req->param("clientPar","jobout");

	file(req,errText);
}

void ServerHandler::manual(VTask_ptr req)
{
	std::string errText="no manual ...";
	req->param("clientPar","manual");

	file(req,errText);
}

void ServerHandler::file(VTask_ptr task,const std::string& errText)
{
	std::string fileName;
	if(!task->param("ecfVar").empty())
	{
		task->node()->findGenVariableValue(task->param("ecfVar"),fileName);
	}

	//We try to read the file from the local disk
	if(task->reply()->textFromFile(fileName))
	{
			task->status(VTask::FINISHED);
			return;
	}
	//If not on local disc we try the client invoker
	else
	{
		// set up and run the thread for server communication
		comQueue_->addTask(task);
	}
}

void ServerHandler::stats(VTask_ptr task)
{
	//comThread()->sendCommand(this,client_,ServerComThread::STATS,req,reply);
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
	int err = 0; // do we need this?

	//We add and update task to the queue.
	comQueue_->addNewsTask();

	/*
	// do not try to update if already updating

	if (updating_)
		return 0;



	// we trigger a refresh by asking for the news; then 
	// ServerHandler::commandSent() handles the rest of the communication

	setUpdatingStatus(true);
	comThread()->sendCommand(this, client_, ServerComThread::NEWS);

	int err = 0; // do we need this?
*/

/*
	// do not try to update if already updating
	if (updating_)
		return 0;

	setUpdatingStatus(true);;
	//SelectNode select(this);

	int err = -1;

	//if (!connected_) return err;

	//gui::watch (True);
	//last_ = ::time(0);

	client_->news_local(); // call the server
	//if (tree_) tree_->connected(True);


	switch ( client_->server_reply().get_news() )
	{
		case ServerReply::NO_NEWS:
			std::cout << "No news from server" << std::endl;
			setUpdatingStatus(false);
			//MainWindow::reload();
			return 0;
			break;



		case ServerReply::NEWS:
			client_->sync_local();
			setUpdatingStatus(false);
			//MainWindow::reload();
			return 0;
			break;


		default:
			break;
	}

	//MainWindow::reload();
*/
/*
      switch ( client_.server_reply().get_news() ) {
         case ServerReply::NO_NEWS:
            gui::message("::nonews\n");
            if (top_) top_->up_to_date();
            return 0;
            break;
      case ServerReply::DO_FULL_SYNC: // 4 calls to the server:

	gui::message("::fullsync\n");
	if (top_) top_->up_to_date();
	update_reg_suites(true);
	reset(true);
	return 0;
	break;
      case ServerReply::NO_DEFS:
	reset(true);
	return 0;
	break;
      case ServerReply::NEWS:
            // there were some kind of changes in the server
            // request the changes from the server & sync with
            // defs on client_
            client_.sync_local();
	    // full_sync==true:  no notification on the GUI side

	    // full_sync==false: incremental change, notification
	    // received through ::update (ecf_node)

	    gui::message("%s: receiving status", name());

	    if (client_.server_reply().full_sync()) {
	      update_reg_suites(false); // new suite may have been added
	      reset(false, false); // SUP-398
	    } else {
              gui::message("%s: updating status", name());
              XECFDEBUG std::cout << "# " << name() << ": small update\n";
              if (tree_)
                tree_->update_tree(false); // fp:60043 Issue with Ecflow updating on console VM
              // redraw(false); // too much blinking with this
            }
            err = 0;
            break;
         default:
            break;
      }
   } catch ( std::exception& e ) {
      if (tree_ != 0x0) tree_->connected(False);
      err = -1;
      gui::message("host::news-error: %s",e.what());
      XECFDEBUG std::cerr << "# host::news-error: " << e.what() << "\n";
   }*/

   /*setUpdatingStatus(false);*/
   return err;
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
			nodeFullName = info->node()->absNodePath();
			nodeName = info->node()->name();
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
			std::string nodeName = info[i]->node()->name();

			//Get the name
			if(info[i]->isNode())
			{
				nodeFullName = info[i]->node()->absNodePath();
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

//---------------------------------------------------------------------------
// Manages node changes and node observers. Node observers are notified when
// any of the nodes changes.
//---------------------------------------------------------------------------

//This slot is called when a node changes.
void ServerHandler::slotNodeChanged(const Node* n, const std::vector<ecf::Aspect::Type>& a)
{
	for(std::vector<NodeObserver*>::const_iterator it=nodeObservers_.begin(); it != nodeObservers_.end(); it++)
		(*it)->notifyNodeChanged(n,a);
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
					break;
				}

				case ServerReply::NEWS:
				{
					// yes, something's changed - synchronise with the server

					UserMessage::message(UserMessage::DBG, false, std::string("News from server - send sync command"));
					comQueue_->addSyncTask(); //comThread()->sendCommand(this, client_, ServerComThread::SYNC);
					break;
				}

				default:
				{
					break;
				}
			}
			break;
		}

		case VTask::ScriptTask:
		{
			task->reply()->text(serverReply.get_string());
			task->status(VTask::FINISHED);
			break;
		}
		case VTask::MessageTask:
		{
			task->reply()->text(serverReply.get_string());
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
		wait_(false)
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
	if(comThread_)
		delete comThread_;
}

void ServerComQueue::addTask(VTask_ptr task)
{
	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::addNewsTask()
{
	VTask_ptr task=VTask::create(VTask::NewsTask);
	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::addSyncTask()
{
	VTask_ptr task=VTask::create(VTask::SyncTask);
	tasks_.push_back(task);
	if(!timer_->isActive())
	{
		timer_->start(0);
	}
}

void ServerComQueue::slotRun()
{
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
	//If the current task is empty there must have been an error that was
	//handled by the sloTaskFailed slot.
	if(!current_)
		return;

	//We notify the server that the task has finished and the results can be accessed.
	server_->clientTaskFinished(current_,client_->server_reply());

	//We do not need the current task any longer.
	current_.reset();
}

//This slot is called when the task failed in the ComThread. Right after this signal is emitted
//the thread will finish and and emits the finished() signal that is connected
//to the slotTaskFinished slot.
void ServerComQueue::slotTaskFailed(std::string msg)
{
	current_->reply()->errorText(msg);
	current_->status(VTask::ABORTED);

	//We do not need the current task any longer.
	current_.reset();
}


//==============================================================
//
//   ServerComThread
//
//==============================================================

ServerComThread::ServerComThread(ServerHandler *server, ClientInvoker *ci) :
		server_(server),
		ci_(ci),
		taskType_(VTask::NoTask)
{
	initObserver(server_);
}

/*void ServerComThread::setCommandString(const std::vector<std::string> command)
{
	command_ = command;
}*/

/*void ServerComThread::sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType)
{
	// do not execute thread if already running

	if (isRunning())
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("ServerComThread::sendCommand - thread already running, will not execute command"));
	}
	else
	{

		if(!server_ && server)
			initObserver(server);

		server_  = server;
		ci_      = ci;
		comType_ = comType;
		start();  // start the thread execution
	}
}

void ServerComThread::sendCommand(ServerComThread::ComType comType,VTask_ptr query,VReply_ptr reply)
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

		server_  = server;
		ci_      = ci;
		comType_ = comType;
		query_=query;
		reply_=reply;

		start();  // start the thread execution
	}
}
*/

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
		if(task->node())
		{
			nodePath_=task->node()->absNodePath();
		}

		//Start the thread execution
		start();
	}
}

void ServerComThread::run()
{
	//Can we use it? We are in the thread!!!
	//UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run start"));

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

/*
void ServerComThread::run()
{
	UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run start"));

	try
 	{
		switch (comType_)
		{
			case COMMAND:
			{
				// call the client invoker with the saved command
				UserMessage::message(UserMessage::DBG, false, std::string("    COMMAND"));
				ArgvCreator argvCreator(command_);
				ci_->invoke(argvCreator.argc(), argvCreator.argv());

				break;
			}

			case NEWS:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    NEWS"));
				ci_->news_local(); // call the server
				//reply_->text(ci_->server_reply().get_news());
				break;
			}

			case SYNC:
			{
				ServerDefsAccess defsAccess(server_);
				UserMessage::message(UserMessage::DBG, false, std::string("    SYNC"));
				ci_->sync_local();
				break;
			}

			case FILE:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    FILE"));
				ci_->file(query_->node()->absNodePath(),query_->param("clientPar"));
				reply_->text(ci_->server_reply().get_string());
				//reply_->done(true);
				break;
			}

			case HISTORY:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    HISTORY"));
				ci_->edit_history(query_->node()->absNodePath());
				reply_->text(ci_->server_reply().get_string_vec());
				//reply_->done(true);
				break;
			}

			case STATS:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    STATS"));

				std::stringstream ss;
				ci_->stats();
				ci_->server_reply().stats().show(ss);
				reply_->text(ss.str());
				reply_->status(VReply::TaskDone);
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

		std::string message = e.what();
		emit errorMessage(message);
	}


	UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run inished"));
	emit taskFinished(reply_);

	//We do not need to held these
	task_.reset();
	reply_.reset();
}
*/

//This is a headache!!!!!!!!!!


void ServerComThread::initObserver(ServerHandler* server)
{
	ServerDefsAccess defsAccess(server);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	int cnt=0;

	ChangeMgrSingleton::instance()->attach(d.get(),this);

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		ChangeMgrSingleton::instance()->attach(suites.at(i).get(),this);

		std::set<Node*> nodes;
		suites.at(i)->allChildren(nodes);
		for(std::set<Node*>::iterator it=nodes.begin(); it != nodes.end(); it++)
			ChangeMgrSingleton::instance()->attach((*it),this);

		cnt+=nodes.size();
	}

	UserMessage::message(UserMessage::DBG, false,std::string("Total number of nodes observed: ") +
			   boost::lexical_cast<std::string>(cnt));
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


VNode::VNode(Node* node) : node_(node)
{
}

void VNode::addChild(VNode* n)
{
    children_.push_back(n);
} 
    
