 //============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <QMessageBox>

#include "ServerHandler.hpp"

#include "Defs.hpp"
#include "ChangeMgrSingleton.hpp"
#include "ClientInvoker.hpp"
#include "File.hpp"
#include "ArgvCreator.hpp"
#include "Str.hpp"
#include "MainWindow.hpp"
#include "UserMessage.hpp"

#include <iostream>

#include <boost/algorithm/string/predicate.hpp>

std::vector<ServerHandler*> ServerHandler::servers_;
std::map<std::string, std::string> ServerHandler::commands_;


ServerHandler::ServerHandler(const std::string& name, const std::string& port) :
   name_(name),
   port_(port),
   client_(0),
   updating_(false),
   communicating_(false),
   comThread_(0),
   refreshIntervalInSeconds_(10)
{
	longName_=name_ + "@" + port_;

	client_=new ClientInvoker(name,port);
	//client_->allow_new_client_old_server(1);
	//client_->allow_new_client_old_server(9);

	std::string server_version;
	client_->server_version();
	server_version = client_->server_reply().get_string();
	UserMessage::message(UserMessage::DBG, false, std::string("ecflow server version: ") + server_version);

	client_->sync_local();

	//Set server name and port in defs
	{
		ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if(defs != NULL)
		{
			ServerState& st=defs->set_server();
			st.hostPort(std::make_pair(name_,port_));
		}
	}


	// we'll need to pass std::strings via signals and slots for error messages
	if (servers_.empty())
	{
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<NodeInfoQuery_ptr>("NodeInfoQuery_ptr");
		qRegisterMetaType<QList<ecf::Aspect::Type> >("QList<ecf::Aspect::Type>");
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
	comThread_ = new ServerComThread();
	connect(comThread(), SIGNAL(finished()), this, SLOT(commandSent()));
	connect(comThread(), SIGNAL(errorMessage(std::string)), this, SLOT(errorMessage(std::string)));
	connect(comThread(), SIGNAL(queryFinished(NodeInfoQuery_ptr)), this, SLOT(queryFinished(NodeInfoQuery_ptr)));


	// set the timer for refreshing the server info
   	connect(&refreshTimer_, SIGNAL(timeout()), this, SLOT(refreshServerInfo()));
	resetRefreshTimer();
}

ServerHandler::~ServerHandler()
{
	if(client_)
		delete client_;

	if (comThread_)
		delete comThread_;
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


ServerHandler* ServerHandler::addServer(const std::string& name, const std::string& port)
{
		ServerHandler* sh=new ServerHandler(name,port);
		return sh;
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

const std::vector<std::string>& ServerHandler::messages(Node* node)
{
	try
	{
	      client_->edit_history(node->absNodePath());
	}
	catch ( std::exception &e )
	{
	      //gui::message("host::messages: %s", e.what());
	}
	return client_->server_reply().get_string_vec();
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


void ServerHandler::queryFinished(NodeInfoQuery_ptr reply)
{
	reply->sender()->queryFinished(reply);
}

void ServerHandler::query(NodeInfoQuery_ptr req)
{
	switch(req->type())
	{
	case NodeInfoQuery::SCRIPT:
		return script(req);
		break;
	case NodeInfoQuery::JOB:
		return job(req);
		break;
	case NodeInfoQuery::JOBOUT:
		return jobout(req);
		break;
	case NodeInfoQuery::MANUAL:
		return manual(req);
		break;
	case NodeInfoQuery::MESSAGE:
		return messages(req);
		break;
	}
}

void ServerHandler::messages(NodeInfoQuery_ptr req)
{
	comThread()->sendCommand(this,client_,ServerComThread::HISTORY,req);
}

void ServerHandler::script(NodeInfoQuery_ptr req)
{
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	req->ecfVar("ECF_SCRIPT");
	req->ciPar("script");

	file(req,errText);
}

void ServerHandler::job(NodeInfoQuery_ptr req)
{
	static std::string errText="no script!\n"
		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
		      		"check for file presence and read access below files directory\n"
		      		"or this may be a 'dummy' task.\n";

	req->ecfVar("ECF_JOB");
	req->ciPar("job");

	file(req,errText);
}

void ServerHandler::jobout(NodeInfoQuery_ptr req)
{
	static std::string errText="no job output...";

	req->ecfVar("ECF_JOBOUT");
	req->ciPar("jobout");

	file(req,errText);
}

void ServerHandler::manual(NodeInfoQuery_ptr req)
{
	std::string errText="no manual ...";
	req->ciPar("manual");

	file(req,errText);
}

void ServerHandler::file(NodeInfoQuery_ptr req,const std::string& errText)
{
	std::string fileName;
	if(!req->ecfVar().empty())
	{
		req->node()->findGenVariableValue(req->ecfVar(),fileName);
		req->fileName(fileName);
	}

	//We try to read the file from the local disk
	if(req->readFile())
	{
	  		req->done(true);
			emit queryFinished(req);
			return;
	}
	//If not on local disc we try the client invoker
	else
	{
		// set up and run the thread for server communication
		comThread()->sendCommand(this,client_,ServerComThread::FILE,req);
	}
}

bool ServerHandler::readFile(Node *n,const std::string& id,
		     std::string& fileName,std::string& txt,std::string& errTxt)
{
  	if(id == "ECF_SCRIPT")
  	{
    	errTxt = "no script!\n"
      		"check ECF_FILES or ECF_HOME directories, for read access\n"
      		"check for file presence and read access below files directory\n"
      		"or this may be a 'dummy' task.\n";

    	n->findGenVariableValue(id,fileName);
  	}
  	else if(id == "ECF_JOB")
  	{
  		n->findGenVariableValue(id,fileName);

  		if(std::string::npos != fileName.find(".job0"))
  	    {
  				errTxt = "job0: no job to be generated yet!";
  				return false;
  	    }

  		errTxt = "no script!\n"
  		      		"check ECF_FILES or ECF_HOME directories, for read access\n"
  		      		"check for file presence and read access below files directory\n"
  		      		"or this may be a 'dummy' task.\n";

  	}
  	else if(boost::algorithm::ends_with(id, ".0"))
  	{
    	errTxt = "no output to be expected when TRYNO is 0!\n";
    	return false;
  	}

  	//Try to read file
  	if(ecf::File::open(fileName,txt))
  	{
  		return true;
  	}
  	else
  	{
  		//gui::message("%s: fetching %s", this->name(), name.c_str());
  	    try
  	    {
  	    	if (id == "ECF_SCRIPT")
  	      			client_->file(n->absNodePath(), "script");
  	    	else if (id == "ECF_JOB")
  	      	{
  	      		client_->file(n->absNodePath(), "job");
  	      		//boost::lexical_cast<std::string>(jobfile_length_));
  	      	}
  	      	else if (id == "ECF_JOBOUT")
  	      	{
  	      		std::cout << "jobout " << n->absNodePath() << std::endl;
  	      		client_->file(n->absNodePath(), "jobout");
  	      	}
  	      	else
  	      	{
  	      		client_->file(n->absNodePath(), "jobout");
  	      	}

  	      	// Do *not* assign 'client_.server_reply().get_string()' to a separate string, since
  	      	// in the case of job output the string could be several megabytes.
  	      	txt=client_->server_reply().get_string();

  	      	std::cout << "txt " << txt << std::endl;

  	      	//return tmp_file(client_->server_reply().get_string());
  	   }
  	   catch(std::exception &e )
  	   {
  	         //gui::message("host::file-error: %s", e.what());
  		   	return false;
  	   }
  	}

  	return true;

  	/*
  	else if(id == "ECF_JOB")
  	{
    	n->findGenVariableValue(name,fileName);

    	if (read && (access(fileName.c_str(), R_OK) == 0))
    		//return tmp_file(fileName.c_str(), false);
    		return true;

    	if(std::string::npos != fileName.find(".job0"))
    	{
			error = "job0: no job to be generated yet!";
			return false;
      	}
      	else
	  	{
	  			error = "no script!\n"
      			"check ECF_HOME,directory for read/write access\n"
      			"check for file presence and read access below\n"
      			"The file may have been deleted\n"
      			"or this may be a 'dummy' task.\n";
      	}
  	}
  	else if(boost::algorithm::ends_with(name, ".0"))
  	{
    	error = "no output to be expected when TRYNO is 0!\n";
    	return false;
  	}
  	else //if (name != ecf_node::none())
  	{
  		// Try logserver
      	loghost_ = n.variable("ECF_LOGHOST", true);
      	logport_ = n.variable("ECF_LOGPORT");
      	if (loghost_ == ecf_node::none())
      	{
         	loghost_ = n.variable("LOGHOST", true);
         	logport_ = n.variable("LOGPORT");
    	}

   		std::string::size_type pos = loghost_.find(n.variable("ECF_MICRO"));
      	if (std::string::npos == pos && loghost_ != ecf_node::none())
      	{
         	logsvr the_log_server(loghost_, logport_);
         	if (the_log_server.ok())
         	{
            	tmp_file tmp = the_log_server.getfile(name); // allow more than latest output
            	if (access(tmp.c_str(), R_OK) == 0) return tmp;
         	}
      	}


  /* if (read && (access(name.c_str(), R_OK) == 0))
   {
		return tmp_file(name.c_str(), false);
   }
   else
   {
      	//gui::message("%s: fetching %s", this->name(), name.c_str());
      	try
      	{
      		if (name == "ECF_SCRIPT")
      			client_->file(n->absNodePath(), "script");
      		else if (name == "ECF_JOB")
      		{
      			client_->file(n->absNodePath(), "job");
      			//boost::lexical_cast<std::string>(jobfile_length_));
      		}
      		else if (name == "ECF_JOBOUT")
      		{
      			client_->file(n->absNodePath(), "jobout");
      		}
      		else
      		{
      			client_->file(n->absNodePath(), "jobout");
      		}

      		// Do *not* assign 'client_.server_reply().get_string()' to a separate string, since
      		// in the case of job output the string could be several megabytes.
      		return tmp_file(client_->server_reply().get_string());
      	}
      	catch ( std::exception &e )
      	{
         //gui::message("host::file-error: %s", e.what());
      	}
   }

   //return tmp_file(error);*/
}

bool ServerHandler::readManual(Node *n,std::string& fileName,std::string& txt,std::string& errTxt)
{
   //gui::message("%s: fetching manual", name());
	try
	{
		client_->file(n->absNodePath(), "manual");
		txt=client_->server_reply().get_string();
		if(txt.empty())
		{
			errTxt = "no manual...";
			return false;
		}
		return true;
	}
	catch ( std::exception &e )
	{
		//gui::message("host::manual-error: %s", e.what());
	}

	errTxt = "no manual...";
	return false;
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

	// do not try to update if already updating

	if (updating_)
		return 0;



	// we trigger a refresh by asking for the news; then 
	// ServerHandler::commandSent() handles the rest of the communication

	setUpdatingStatus(true);
	comThread()->sendCommand(this, client_, ServerComThread::NEWS);

	int err = 0; // do we need this?


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

   setUpdatingStatus(false);
   return err;
}

void ServerHandler::refreshServerInfo()
{
	UserMessage::message(UserMessage::DBG, false, std::string("auto refreshing server info for ") + name());
	update();
}


ServerComThread *ServerHandler::comThread()
{
	return comThread_;
}

void ServerHandler::command(std::vector<ViewNodeInfo_ptr> info, std::string cmd, bool resolve)
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

		for(int i=0; i < info.size(); i++)
		{
			std::string nodeFullName;
			std::string nodeName = info[i]->node()->name();

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

			info[i]->server()->targetNodeNames_     += " " + nodeName;      // build up the list of nodes for each server
			info[i]->server()->targetNodeFullNames_ += " " + nodeFullName;  // build up the list of nodes for each server


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
			ecf::Str::replace_all(realCommand, placeholder, serverHandler->targetNodeFullNames_);

			placeholder = "<node_name>";
			ecf::Str::replace_all(realCommand, placeholder, serverHandler->targetNodeNames_);

			UserMessage::message(UserMessage::DBG, false, std::string("final command: ") + realCommand);


			// get the command into the right format by first splitting into tokens
			// and then converting to argc, argv format

			std::vector<std::string> strs;
			std::string delimiters(" ");
			ecf::Str::split(realCommand, strs, delimiters);

			// set up and run the thread for server communication
			serverHandler->comThread()->setCommandString(strs);
			serverHandler->comThread()->sendCommand(serverHandler, serverHandler->client_, ServerComThread::COMMAND);


			serverHandler->targetNodeNames_.clear();      // reset the target node names for next time
			serverHandler->targetNodeFullNames_.clear();  // reset the target node names for next time
			//serverHandler->update();
		}
	}
	else
	{
		UserMessage::message(UserMessage::ERROR, true, std::string("command ") + cmd + " is not recognised. Check the menu definition.");
	}
}


ServerHandler* ServerHandler::find(const std::string& longName)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
			if((*it)->longName() == longName)
					return *it;
	return NULL;
}

ServerHandler* ServerHandler::find(const std::pair<std::string,std::string>& hostPort)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
			if((*it)->name_ == hostPort.first && (*it)->port_ == hostPort.second)
					return *it;
	return NULL;
}

ServerHandler* ServerHandler::find(const std::string& name,const std::string& port)
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
			if((*it)->name_ == name && (*it)->port_ == port)
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
			return ServerHandler::find(st.hostPort());
		}
	}
	return NULL;
}


void ServerHandler::addNodeObserver(QObject* obs)
{
		connect(comThread_,SIGNAL(nodeChanged(const Node*, QList<ecf::Aspect::Type>)),
				obs,SLOT(slotNodeChanged(const Node*, QList<ecf::Aspect::Type>)));

}

void ServerHandler::removeNodeObserver(QObject* obs)
{
		disconnect(comThread_,SIGNAL(nodeChanged(const Node*, QList<ecf::Aspect::Type>)),
				obs,SLOT(slotNodeChanged(const Node*, QList<ecf::Aspect::Type>)));
}


// called by ChangeMgrSingleton when the definition is about to be updated
/*void ServerHandler::update(const Defs*, const std::vector<ecf::Aspect::Type>&)
{
}*/



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




	/*client_=new ClientInvoker(server, port);
    client_->allow_new_client_old_server(1);

    std::string server_version;
    client_->server_version();
    server_version = client_->server_reply().get_string();
    std::cout << "ecflow server version: " << server_version << "\n";

    client_->sync_local();*/
    //defs_ptr defs = client_.defs();

    //const std::vector<suite_ptr> &suites = defs->suiteVec();


//const std::vector<suite_ptr> suites

/*
	size_t numSuites = suites.size();
    std::cout << "Num suites: " << numSuites << std::endl;
	for (size_t s = 0; s < numSuites; s++)
    {
        QString suiteName(suites[s]->name().c_str());
        QTreeWidgetItem *suiteItem = new QTreeWidgetItem;
        suiteItem->setText(s, suiteName);
        treeWidget_->insertTopLevelItem(s, suiteItem);
        suiteItem->setExpanded(true);

        const std::vector<node_ptr> &nodes = suites[s]->nodeVec();
        for (size_t n = 0; n < nodes.size(); n++)
        {
            printNode(nodes[n], 2, suiteItem);
        }
    }
 */



void ServerHandler::commandSent()
{
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


// ------------------------------------------------------------
//                         ServerComThread
// ------------------------------------------------------------


ServerComThread::ServerComThread() :
		server_(0),
		ci_(0)
{
}

void ServerComThread::setCommandString(const std::vector<std::string> command)
{
	command_ = command;
}

void ServerComThread::sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType)
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

void ServerComThread::sendCommand(ServerHandler *server, ClientInvoker *ci, ServerComThread::ComType comType,
									NodeInfoQuery_ptr query)
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

		start();  // start the thread execution
	}
}



ServerComThread::ComType ServerComThread::commandType()
{
	return comType_;
}

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
				ci_->file(query_->node()->absNodePath(),query_->ciPar());
				query_->text(ci_->server_reply().get_string());
				query_->done(true);
				break;
			}

			case HISTORY:
			{
				UserMessage::message(UserMessage::DBG, false, std::string("    HISTORY"));
				ci_->edit_history(query_->node()->absNodePath());
				query_->text(ci_->server_reply().get_string_vec());
				query_->done(true);
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

	if(comType_ == FILE || comType_ == HISTORY)
	{
		emit queryFinished(query_);
		query_.reset();
	}


	UserMessage::message(UserMessage::DBG, false, std::string("  ServerComThread::run finished"));
}

void ServerComThread::initObserver(ServerHandler* server)
{
	ServerDefsAccess defsAccess(server);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d == NULL)
		return;

	const std::vector<suite_ptr> &suites = d->suiteVec();
	for(unsigned int i=0; i < suites.size();i++)
	{
		ChangeMgrSingleton::instance()->attach(suites.at(i).get(),this);

		std::set<Node*> nodes;
		suites.at(i)->allChildren(nodes);
		for(std::set<Node*>::iterator it=nodes.begin(); it != nodes.end(); it++)
			ChangeMgrSingleton::instance()->attach((*it),this);

	}
}

void ServerComThread::update(const Node* node, const std::vector<ecf::Aspect::Type>& types)
{
	if(node==NULL)
		return;

	QList<ecf::Aspect::Type> v;
	for(std::vector<ecf::Aspect::Type>::const_iterator it=types.begin(); it != types.end(); it++)
			v << *it;

	//UserMessage::message(UserMessage::DBG, false, std::string("Thread update: ") + node->name());

	emit nodeChanged(node,v);
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

