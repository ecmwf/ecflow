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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "MyDefsFixture.hpp"
#include "Ecf.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_FIXTURE_TEST_SUITE( NodeTestSuite, MyDefsFixture )

// Allow for multiple archives
static void testPersistence(const Defs& fixtureDefs)
{
	std::string check_pt_file = "fixture_defs.check";
	fixtureDefs.cereal_save_as_checkpt(check_pt_file);
	std::string error_msg;
	BOOST_CHECK_MESSAGE(fixtureDefs.checkInvariants(error_msg),error_msg);

	Defs restoredDefs;
	restoredDefs.cereal_restore_from_checkpt(check_pt_file);
	error_msg.clear();
   BOOST_CHECK_MESSAGE(restoredDefs.checkInvariants(error_msg),error_msg);

	bool theyCompare = (restoredDefs == fixtureDefs);
	if (!theyCompare) {

		std::cout << "Dump restored defs\n" << restoredDefs << "\n";
		std::cout << "Dump fixture defs\n" << fixtureDefs << "\n";

		BOOST_CHECK_MESSAGE(theyCompare,"restored defs file is not same as fixtureDefs defs file");
	}

	cout << " check pt file_size: " <<  fs::file_size(check_pt_file) << "\n";

 	// Uncomment if you want see what this file looks like
	fs::remove(check_pt_file);
}

BOOST_AUTO_TEST_CASE( test_node_tree_persistence_text )
{
   cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_text" ;
   BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
   testPersistence(fixtureDefsFile());
}


BOOST_AUTO_TEST_CASE( test_node_defs_persistence  )
{
   cout << "ANode:: ...test_node_defs_persistence\n" ;

   const Defs& defs = fixtureDefsFile();
   std::vector<node_ptr> all_nodes;
   defs.get_all_nodes(all_nodes);
   BOOST_REQUIRE_MESSAGE(all_nodes.size()>0,"Expected nodes");
   for(auto & all_node : all_nodes) {
      std::string node_as_defs_string = all_node->print(PrintStyle::MIGRATE);
      node_ptr the_copy = Node::create(node_as_defs_string);
      BOOST_REQUIRE_MESSAGE(the_copy,"Failed to create node " << all_node->absNodePath() << " from string " << node_as_defs_string);
      BOOST_REQUIRE_MESSAGE(*the_copy == *all_node,"Nodes not the same");
   }
}
BOOST_AUTO_TEST_SUITE_END()
