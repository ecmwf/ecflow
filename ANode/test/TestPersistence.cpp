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
static void testPersistence(const Defs& fixtureDefs,ecf::Archive::Type at)
{
	std::string check_pt_file = "fixture_defs.check";
	fixtureDefs.boost_save_as_checkpt(check_pt_file,at);

	Defs restoredDefs;
	restoredDefs.boost_restore_from_checkpt(check_pt_file,at);

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

#if defined(BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_binary )
{
	cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_binary";
	BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
	testPersistence(fixtureDefsFile(),ecf::Archive::BINARY);
}
#elif defined(PORTABLE_BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_portable_binary )
{
	cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_portable_binary";
	BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
	testPersistence(fixtureDefsFile(),ecf::Archive::PORTABLE_BINARY);
}
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_eos_portable_binary )
{
   cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_eos_portable_binary";
   BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
   testPersistence(fixtureDefsFile(),ecf::Archive::EOS_PORTABLE_BINARY);
}
#else
BOOST_AUTO_TEST_CASE( test_node_tree_persistence_text )
{
   cout << left << setw(54) << "ANode:: ...test_node_tree_persistence_text" ;
   BOOST_CHECK_MESSAGE(true,""); // stop boost complaining about no assertions
   testPersistence(fixtureDefsFile(),ecf::Archive::TEXT);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
