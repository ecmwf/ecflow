/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"
#include "PrintStyle.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_find_abs_node_path )
{
   cout << "ANode:: ...test_find_abs_node_path\n";

   size_t no_of_nodes = 0;
   size_t no_of_alias = 0;
   Defs theDefs;
   {
      for(int s = 0; s < 3; s++) {
         suite_ptr suite = theDefs.add_suite( "suite" + boost::lexical_cast<std::string>(s));  no_of_nodes++;
         for(int f = 0; f < 3; f++) {
            family_ptr fam = suite->add_family( "family" + boost::lexical_cast<std::string>(f));  no_of_nodes++;
            for(int ff = 0; ff < 3; ff++) {
                family_ptr hfam = fam->add_family( "family" + boost::lexical_cast<std::string>(ff));  no_of_nodes++;
                for(int t = 0; t < 3; t++) {
                   task_ptr task = hfam->add_task( "t1" + boost::lexical_cast<std::string>(t));  no_of_nodes++;
                   for(int a = 0; a < 3; a++) {
                      task->add_alias_only();  no_of_nodes++; no_of_alias++;
                   }
                }
            }
          }
      }
   }

   // Test Defs::getAllNodes()
   std::vector<Node*> all_nodes;
   theDefs.getAllNodes(all_nodes);
   BOOST_CHECK_MESSAGE(all_nodes.size() == no_of_nodes,"Expected theDefs.getAllNodes() to return " << no_of_nodes << " node, but found " << all_nodes.size());

   // Test Defs::get_all_aliases()
   std::vector<alias_ptr> alias_vec;
   theDefs.get_all_aliases(alias_vec);
   BOOST_CHECK_MESSAGE(alias_vec.size() == no_of_alias,"Expected theDefs.get_all_aliases() to return " << no_of_alias << " node, but found " << alias_vec.size());

   // Test Defs::findAbsNode()
//   PrintStyle style(PrintStyle::STATE); std::cout << theDefs;
   for(size_t i= 0; i < all_nodes.size(); i++) {
      Node* node = all_nodes[i];
      node_ptr found_node = theDefs.findAbsNode(node->absNodePath());
      BOOST_CHECK_MESSAGE(found_node.get(),"Could not find node " <<  node->debugNodePath());
      BOOST_CHECK_MESSAGE(found_node.get() == node," Expected to find " <<  node->debugNodePath() << " but found " << found_node->debugNodePath());
   }
}

BOOST_AUTO_TEST_SUITE_END()
