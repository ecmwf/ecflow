#if defined(TEXT_ARCHIVE) || !defined(BINARY_ARCHIVE) && !defined(PORTABLE_BINARY_ARCHIVE) && !defined(EOS_PORTABLE_BINARY_ARCHIVE)
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"
#include "Ecf.hpp"
#include "SerializationTest.hpp"
#include "MyDefsFixture.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;
namespace fs = boost::filesystem;

// If you are updating the tests, *MAKE SURE* to check out test/data/migration/* files
//#define UPDATE_TESTS 1

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

//
// These test are used for future release. They help to ensure that we have
// backward compatibility.i.e future release can open file, created by an earlier release
//
BOOST_AUTO_TEST_CASE( test_migration_restore_def_con_3_0_1 )
{
   cout << "ANode:: ...test_migration_restore_def_con_3_0_1\n";

   std::string file_name = File::test_data("ANode/test/data/migration/default_constructor/","ANode");

   // Create migration data
   Defs defs;
   Suite suite;
   Family family;
   Task   task;

   // Remove host dependent variables from server state, so that we can run on other platforms
   defs.set_server().delete_server_variable("ECF_LOG");
   defs.set_server().delete_server_variable("ECF_CHECK");
   defs.set_server().delete_server_variable("ECF_CHECKOLD");

   // We use .def extension so that we copy over writable files with extension .def to
   // other platforms during incremental build's
#ifdef UPDATE_TESTS
   // remember to check out data
   doSave(file_name + "Defs.def",defs);
   doSave(file_name + "Suite.def",suite);
   doSave(file_name + "Family.def",family);
   doSave(file_name + "Task.def",task);
   doSave(file_name + "Limit.def",Limit());
#else
   Ecf::set_debug_equality(true);
   do_restore<Defs>(file_name + "Defs.def",defs);
   do_restore<Suite>(file_name + "Suite.def",suite);
   do_restore<Family>(file_name + "Family.def",family);
   do_restore<Task>(file_name + "Task.def",task);
   do_restore<Limit>(file_name + "Limit.def",Limit());
   Ecf::set_debug_equality(false);
#endif
}

//#define UPDATE_TESTS 1

BOOST_AUTO_TEST_CASE( test_migration_restore_boost_1_47_checkpt_file )
{
   cout << "ANode:: ...test_migration_restore_boost_1_47_checkpt_file\n";

   std::string file_name = File::test_data("ANode/test/data/migration/fixture/","ANode");

   // Create migration data
   // This will create a pre-built definition.
   // If the definition is changed we will need to update this test.
   // **Keep*** old checkpt test data, to ensure future ecflow versions can be migrated
   // **Update** here for future boost updates
   // **IF MyDefsFixture is changed, we need to ensure we can migrate it to future versions
   MyDefsFixture fixture("boost_1_47.checkpt");

   // Allow the test data, to be used on other platforms
   fixture.remove_host_depedent_server_variables();

#ifdef UPDATE_TESTS
   // remember to check out data
   doSave(file_name + "boost_1_47.checkpt",fixture.fixtureDefsFile());
#else
   Ecf::set_debug_equality(true);
   do_restore<Defs>(file_name + "boost_1_47.checkpt",fixture.fixtureDefsFile());
   Ecf::set_debug_equality(false);
#endif
}


BOOST_AUTO_TEST_SUITE_END()

#endif
