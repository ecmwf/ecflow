/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <iostream>
#include <string>

#include "ecflow/node/Defs.hpp"

using namespace std;
using namespace ecf;

// leap42 boost_1_64_0 gcc-5.3.0 release mode
//    time bin/gcc-5.3.0/release/perf_aparser_only ${ECF_TEST_DEFS_DIR}/vsms2.31415.def
//    real    0m2.79s
//    user    0m2.59s
//    sys     0m0.18s

int main(int argc, char* argv[]) {
    //   cout << "argc = " << argc << "\n";
    //   for(int i = 0; i < argc; i++) {
    //      cout << "arg " << i << ":" << argv[i] << "\n";
    //   }

    if (argc != 2) {
        cout << "Expect single argument which is path to a defs file\n";
        return 1;
    }

    std::string path = argv[1];

    Defs defs;
    std::string errorMsg, warningMsg;
    if (!defs.restore(path, errorMsg, warningMsg)) {
        cout << errorMsg << "\n";
        cout << warningMsg << "\n";
        return 1;
    }

    //   // Determine average number of variables for nodes with variables
    //   vector<Node*> nodes;
    //   defs.getAllNodes(nodes);
    //   cout << "Total number of nodes: " << nodes.size() << "\n";
    //   size_t number_of_variables = 0;
    //   size_t nodes_with_variables = 0;
    //   for(const auto& n : nodes) {
    //      // cout << n->variables().size() << "\n";
    //      if (n->variables().size() > 0) {
    //         number_of_variables +=  n->variables().size();
    //         nodes_with_variables++;
    //      }
    //   }
    //   cout << "number_of_variables " << number_of_variables << "\n";
    //   cout << "nodes_with_variables  " <<  nodes_with_variables << "\n";
    //   cout <<"Average number of variables per node " <<  (double)number_of_variables/nodes_with_variables  << "\n";
    return 0;
}
