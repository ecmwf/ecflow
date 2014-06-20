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
#include "ClientInvoker.hpp"
#include "ArgvCreator.hpp"
#include "Str.hpp"
#include "MainWindow.hpp"

#include <iostream>

std::vector<ServerHandler*> ServerHandler::servers_;
std::map<std::string, std::string> ServerHandler::commands_;

ServerHandler::ServerHandler(const std::string& name, const std::string& port) :
   name_(name),
   port_(port),
   client_(0),
   updating_(false),
   communicating_(false),
   comThread_(0)
{
	longName_=name_ + "@" + port_;

	client_=new ClientInvoker(name,port);
	//client_->allow_new_client_old_server(1);
	//client_->allow_new_client_old_server(9);

	std::string server_version;
	client_->server_version();
	server_version = client_->server_reply().get_string();
	std::cout << "ecflow server version: " << server_version << "\n";

	client_->sync_local();

	//Set server name and pot in defs
	{
		ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
		defs_ptr defs = defsAccess.defs();
		if(defs != NULL)
		{
			ServerState& st=defs->set_server();
			st.hostPort(std::make_pair(name_,port_));
		}
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

}

ServerHandler::~ServerHandler()
{
	if(client_)
		delete client_;

	if (comThread_)
		delete comThread_;

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

Node* ServerHandler::suiteAt(int pos)
{
	ServerDefsAccess defsAccess(this);  // will reliquish its resources on destruction
	defs_ptr d = defsAccess.defs();
	if(d != NULL)
	{
		const std::vector<suite_ptr> &suites = d->suiteVec();
		return (pos >=0 && pos < suites.size())?suites.at(pos).get():NULL;
	}

	return NULL;
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

void ServerHandler::updateAll()
{
	for(std::vector<ServerHandler*>::const_iterator it=servers_.begin(); it != servers_.end();it++)
			(*it)->update();
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


ServerComThread *ServerHandler::comThread()
{
	return comThread_;
}

void ServerHandler::command(std::vector<ViewNodeInfo_ptr> info,std::string cmd)
{
	std::string realCommand = resolveServerCommand(cmd);

	if (!realCommand.empty())
	{
		std::cout << "command: " << cmd << " (real: " << realCommand << ")" << std::endl;


		for(int i=0; i < info.size(); i++)
		{
			std::string nodeName;
			if(info[i]->isNode())
			{
				nodeName = info[i]->node()->absNodePath();
				std::cout << "  --> for node: " << nodeName <<   " (server: " << info[i]->server()->longName() << ")" << std::endl;
			}
			else if(info[i]->isServer())
			{
				nodeName = info[i]->server()->longName();
				std::cout << "  --> for server: " << nodeName << std::endl;
			}

			std::string placeholder("<full_name>");
			size_t pos = realCommand.find(placeholder);
			if (pos != std::string::npos)
			{
				realCommand.replace(pos,placeholder.length(),nodeName);
				std::cout << "final command: " << realCommand << std::endl;
			}


			// get the command into the right format by first splitting into tokens
			// and then converting to argc, argv format

			std::vector<std::string> strs;
			std::string delimiters(" ");
			ecf::Str::split(realCommand, strs, delimiters);
			ServerHandler* serverHandler = info[i]->server();

			// set up and run the thread for server communication
			serverHandler->comThread()->setCommandString(strs);
			serverHandler->comThread()->sendCommand(serverHandler, serverHandler->client_, ServerComThread::COMMAND);

			//serverHandler->update();
		}
	}
	else
	{
		// XXX TODO: inform the user that this command is not recognised
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


// called by ChangeMgrSingleton when the definition is about to be updated
void ServerHandler::update(const Defs*, const std::vector<ecf::Aspect::Type>&)
{
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
		std::cout << " Command: " << name << " is not registered" << std::endl;
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
	std::cout << "ServerHandler::commandSent" << "\n";

	// which type of command was sent? What we do now will depend on that.

	switch (comThread()->commandType())
	{
		case ServerComThread::COMMAND:
		{
			// a command was sent - we should now check whether there have been
			// any updates on the server (there should have been, because we
			// just did something!)

			std::cout << "Send command to server" << std::endl;
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
					std::cout << "No news from server" << std::endl;
					setUpdatingStatus(false);  // finished updating
					break;
				}

				case ServerReply::NEWS:
				{
					// yes, something's changed - synchronise with the server

					std::cout << "News from server - send sync command" << std::endl;
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

			std::cout << "We've synced" << std::endl;
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



ServerComThread::ServerComThread()
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
		std::cout << "ServerComThread::sendCommand - thread already running, will not execute command" << std::endl;
	}
	else
	{
		server_  = server;
		ci_      = ci;
		comType_ = comType;
		start();  // start the thread execution
	}
}

ServerComThread::ComType ServerComThread::commandType()
{
	return comType_;
}

void ServerComThread::run()
{

	std::cout << "  ServerComThread::run start" << "\n";

	switch (comType_)
	{
		case COMMAND:
		{
			// call the client invoker with the saved command
			std::cout << "    COMMAND" << "\n";
			ArgvCreator argvCreator(command_);
			ci_->invoke(argvCreator.argc(), argvCreator.argv());
			break;
		}

		case NEWS:
		{
			std::cout << "    NEWS" << "\n";
			ci_->news_local(); // call the server
			break;
		}

		case SYNC:
		{
			ServerDefsAccess defsAccess(server_);
			std::cout << "    SYNC" << "\n";
			ci_->sync_local();
			break;
		}

		default:
		{
		}
	}
	std::cout << "  ServerComThread::run finished" << "\n";
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

