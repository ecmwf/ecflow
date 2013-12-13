//============================================================================
// Copyright 2013 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
//============================================================================


#include <QtGui>

#include "Defs.hpp"
#include "ClientInvoker.hpp"
#include "ViewerMainWindow.hpp"


ViewerMainWindow::ViewerMainWindow()
{
    textEditor_ = new QPlainTextEdit;
    setCentralWidget(textEditor_);
}



void ViewerMainWindow::printDefTree(const std::string &server, int port)
{
    ClientInvoker client(server, port);
    client.allow_new_client_old_server(1);

    std::string server_version;
    client.server_version();
    server_version = client.server_reply().get_string();
    std::cout << "ecflow server version: " << server_version << "\n";


    client.sync_local();
    defs_ptr defs = client.defs();

    const std::vector<suite_ptr> &suites = defs->suiteVec();


	size_t numSuites = suites.size();
    std::cout << "Num suites: " << numSuites << std::endl;
	for (size_t s = 0; s < numSuites; s++)
    {
        QString suiteName(suites[s]->name().c_str());
        textEditor_->insertPlainText(suiteName + " (SUITE)\n");
        const std::vector<node_ptr> &nodes = suites[s]->nodeVec();
        for (size_t n = 0; n < nodes.size(); n++)
        {
            printNode(nodes[n], 2);
            //std::cout << "  NODE: " << nodes[n]->name() << std::endl;
        }
    }


}


void ViewerMainWindow::printNode(node_ptr node, int indent)
{
    QString spaces;
    for (size_t i = 0; i < indent; i++)
    {
        spaces += "  ";
    }

    QString description;
    if (node->isFamily())
        description += " (FAMILY)";

    if (node->isTask())
        description += " (TASK)";

    QString nodeName(node->name().c_str());
    textEditor_->insertPlainText(spaces + nodeName + description + "\n");

    //NodeContainer *nodeContainer = &node;
    std::vector<node_ptr> nodes;
    node->immediateChildren(nodes);
    for (size_t n = 0; n < nodes.size(); n++) // starts at 1 because it includes the current node
    {
        printNode(nodes[n], indent+2);
    }

}
