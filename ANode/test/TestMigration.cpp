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

//#define UPDATE_TESTS 1

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_default_constructor_persistence )
{
   cout << "ANode:: ...test_default_constructor_persistence\n";

   std::string file_name = File::test_data("ANode/test/data/migration/","ANode");

   Defs defs;
   Suite suite;
   Family family;
   Task   task;

   // Can't persist server variable are dependent on HOST.i.e ECF_LISTS,ECF_CHECK,etc
   // Hence is not cross-platform
   doSave(file_name + "Defs.def",defs);

#ifdef UPDATE_TESTS
   doSave(file_name + "Suite.def",suite);
   doSave(file_name + "Family.def",family);
   doSave(file_name + "Task.def",task);
   doSave(file_name + "Limit.def",Limit());
#endif

   DebugEquality debug_equality; // only as affect in DEBUG build
   do_restore<Defs>(file_name + "Defs.def",defs);
   do_restore<Suite>(file_name + "Suite.def",suite);
   do_restore<Family>(file_name + "Family.def",family);
   do_restore<Task>(file_name + "Task.def",task);
   do_restore<Limit>(file_name + "Limit.def",Limit());

   fs::remove(file_name + "Defs.def" ); // Remove the file. Comment out for debugging
}

BOOST_AUTO_TEST_CASE( test_compare_boost_and_defs_checkpt_file )
{
   cout << "ANode:: ...test_compare_boost_and_defs_checkpt_file\n";

   std::string file_name = File::test_data("ANode/test/data/migration/","ANode");

   // Cannot save these tests since server variable use HOST which is different for each platform
   MyDefsFixture fixture;
   doSave(file_name + "cereal.checkpt",fixture.fixtureDefsFile());
   fixture.fixtureDefsFile().save_as_checkpt(file_name + "defs.checkpt");


   DebugEquality debug_equality; // only as affect in DEBUG build
   do_restore<Defs>(file_name + "cereal.checkpt",fixture.fixtureDefsFile());

   Defs defs;
   defs.restore( file_name + "defs.checkpt" );
   BOOST_CHECK_MESSAGE(defs == fixture.fixtureDefsFile()," ");

   fs::remove(file_name + "defs.checkpt" ); // Remove the file. Comment out for debugging
   fs::remove(file_name + "cereal.checkpt" ); // Remove the file. Comment out for debugging
}

BOOST_AUTO_TEST_SUITE_END()
