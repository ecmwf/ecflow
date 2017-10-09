//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientToServerCmd.hpp"
#include "MyDefsFixture.hpp"
#include "PrintStyle.hpp"
#include "File.hpp"
#include "Ecf.hpp"
#include "Pid.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;


BOOST_AUTO_TEST_SUITE( BaseTestSuite )


BOOST_AUTO_TEST_CASE( test_load_defs_cmd )
{
   cout << "Base:: ...test_load_defs_cmd \n";

   MyDefsFixture fixtureDef ;
   defs_ptr defs = fixtureDef.create_defs();

   // save this defs as defs file format
   std::string test_dir = File::test_data("Base/test","Base");
   std::string defs_format = Pid::unique_name(test_dir + "/test_load_defs_cmd.def");
   std::string boost_format = Pid::unique_name(test_dir + "/test_load_defs_cmd.check");

   defs->save_as_checkpt(  defs_format );
   defs->boost_save_as_checkpt(boost_format);

   BOOST_CHECK_MESSAGE(fs::exists( defs_format  ),   defs_format << " file not created" );
   BOOST_CHECK_MESSAGE(fs::exists(  boost_format  ), boost_format << " file not created" );

   LoadDefsCmd load_as_defs_cmd(defs_format , false /* force */, false /* check_only */);
   LoadDefsCmd load_as_boost_cmd( boost_format , false /* force */, false /* check_only */);

   DebugEquality debug_equality; // only as affect in DEBUG build
   BOOST_CHECK_MESSAGE(*load_as_defs_cmd.theDefs() == *load_as_boost_cmd.theDefs()," Expected boost and defs to be equal");

   fs::remove( defs_format );
   fs::remove( boost_format  );
}

BOOST_AUTO_TEST_SUITE_END()
