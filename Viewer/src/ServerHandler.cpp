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
#include <sstream>

std::vector<ServerHandler*> ServerHandler::servers_;

ServerHandler::ServerHandler(const std::string& name, int port) :
   name_(name),
   port_(port),
   client_(0)
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

	servers_.push_back(this);
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

defs_ptr ServerHandler::defs()
{
	if(client_)
			return client_->defs();
}

void ServerHandler::command(std::vector<ServerHandler*>,std::vector<Node*>,std::string cmd)
{


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
