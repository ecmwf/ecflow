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

#include <iostream>

std::vector<ServerHandler*> ServerHandler::servers_;

ServerHandler::ServerHandler(const std::string& name, const std::string& port) :
   name_(name),
   port_(port),
   client_(0)

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
	defs_ptr defs = client_->defs();
	if(defs != NULL)
	{
		ServerState& st=defs->set_server();
		st.hostPort(std::make_pair(name_,port_));
	}

	servers_.push_back(this);
}

ServerHandler::~ServerHandler()
{
	if(client_)
			delete client_;
}

ServerHandler* ServerHandler::addServer(const std::string& name, const std::string& port)
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

void ServerHandler::command(std::vector<ViewNodeInfo_ptr> info,std::string cmd)
{
	std::cout << "command: " << cmd << std::endl;
	for(int i=0; i < info.size(); i++)
	{
			if(info[i]->isNode())
			{
					std::cout << "  --> for node: " << info[i]->node()->absNodePath() <<   " (server: " << info[i]->server()->longName() << ")" << std::endl;
			}
			else if(info[i]->isServer())
			{
					std::cout << "  --> for server: " << info[i]->server()->longName() << std::endl;
			}
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

ServerHandler* ServerHandler::find(Node *node)
{
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
