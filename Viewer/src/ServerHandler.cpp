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
#include "Str.hpp"
#include "ArgvCreator.hpp"

#include <iostream>
#include <sstream>

std::vector<ServerHandler*> ServerHandler::servers_;
std::map<std::string, std::string> ServerHandler::commands_;

ServerHandler::ServerHandler(const std::string& name, int port) :
   name_(name),
   port_(port),
   client_(0),
   updating_(false)
{
	std::stringstream ss;
	ss << port_;
	longName_=name_ + "@" + ss.str();

	client_=new ClientInvoker(name, port);
	//client_->allow_new_client_old_server(1);
	//client_->allow_new_client_old_server(9);

	std::string server_version;
	client_->server_version();
	server_version = client_->server_reply().get_string();
	std::cout << "ecflow server version: " << server_version << "\n";

	client_->sync_local();

	//Set server name and pot in defs
	defs_ptr defs = client_->defs();
	if(defs != NULL)
	{
		ServerState& st=defs->set_server();
		st.add_or_update_user_variables("longName",longName_);
	}

	servers_.push_back(this);


	// populate the map of server commands - in the future this will be done when
	// the configuration file is parsed
	if (commands_.empty())
	{
		addServerCommand("Requeue", "ecflow_client --requeue force <full_name>");
		addServerCommand("Execute", "ecflow_client --run <full_name>");
	}
}


ServerHandler* ServerHandler::addServer(const std::string& name, int port)
{
		ServerHandler* sh=new ServerHandler(name,port);
		return sh;
}

int ServerHandler::suiteNum()  const
{
	if(client_)
	{
			defs_ptr defs = client_->defs();
			if(defs != NULL)
			{
				return static_cast<int>(defs->suiteVec().size());
			}
	}
	return 0;
}

Node* ServerHandler::suiteAt(int pos) const
{
	defs_ptr d = defs();
	if(d != NULL)
	{
		const std::vector<suite_ptr> &suites = d->suiteVec();
		return (pos >=0 && pos < suites.size())?suites.at(pos).get():NULL;
	}

	return NULL;
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

defs_ptr ServerHandler::defs() const
{
	if(client_)
			return client_->defs();
}


int ServerHandler::numOfImmediateChildren(Node *node)
{
	if(!node) return 0;

	std::vector<node_ptr> nodes;
	node->immediateChildren(nodes);
	return static_cast<int>(nodes.size());
}


// see view/host.cc / ehost::update() for full code
int ServerHandler::update()
{
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
			return 0;
			break;



		case ServerReply::NEWS:
			client_->sync_local();
			return 0;
			break;


		default:
			break;
	}



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
   return err;
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
			ArgvCreator argvCreator(strs);
			ServerHandler* serverHandler = info[i]->server();
			serverHandler->client_->invoke(argvCreator.argc(), argvCreator.argv());

			serverHandler->update();
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

ServerHandler* ServerHandler::find(Node *node)
{
	if(node)
	{
		if(Defs* defs = node->defs())
		{
			const ServerState& st=defs->server();
			return ServerHandler::find(st.find_variable("longName"));
		}
	}
	return NULL;
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
