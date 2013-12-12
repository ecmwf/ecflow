#include <string>
#include <iostream>

#include "Defs.hpp"
#include "ClientInvoker.hpp"
//#include "ClientToServerCmd.hpp"


void printNode(node_ptr node, int indent)
{
    std::string spaces;
    for (size_t i = 0; i < indent; i++)
    {
        spaces += " ";
    }

    std::string description;
    if (node->isFamily())
        description += " (FAMILY)";

    if (node->isTask())
        description += " (TASK)";


    std::cout << spaces << node->name() << description << std::endl;

    //NodeContainer *nodeContainer = &node;
    std::vector<node_ptr> nodes;
    node->get_all_nodes(nodes);
    for (size_t n = 1; n < nodes.size(); n++) // starts at 1 because it includes the current node
    {
        printNode(nodes[n], indent+2);
    }

}


int main(int argc, char **argv)
{

    if (argc != 3)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <host> <port>" << std::endl;
        return 1;
    }

    ClientInvoker client("darth", "16755");
    client.allow_new_client_old_server(1);

    std::string server_version;
    client.server_version();
    server_version = client.server_reply().get_string();
    std::cout << "ecflow server version: " << server_version << "\n";


    //client.suites();
    //const std::vector<std::string>& suites = client.server_reply().get_string_vec();

    //for(size_t i =0; i < suites.size(); i++)
    //{
    //    std::cout << "SUITE: " << suites[i] << "\n";
    //}


    client.sync_local();
    defs_ptr defs = client.defs();

    const std::vector<suite_ptr> &suites = defs->suiteVec();


	size_t numSuites = suites.size();
    std::cout << "Num suites: " << numSuites << std::endl;
	for (size_t s = 0; s < numSuites; s++)
    {
        std::cout << suites[s]->name() << " (SUITE)" << std::endl;
        const std::vector<node_ptr> &nodes = suites[s]->nodeVec();
        for (size_t n = 0; n < nodes.size(); n++)
        {
            printNode(nodes[n], 2);
            //std::cout << "  NODE: " << nodes[n]->name() << std::endl;
        }
    }

    return 0;
}
