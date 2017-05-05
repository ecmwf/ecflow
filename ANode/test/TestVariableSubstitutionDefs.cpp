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

#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "Defs.hpp"
#include "Str.hpp"
#include "Version.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_defs_variable_substitution )
{
    std::cout <<  "ANode:: ...test_defs_variable_substitution\n";

    Defs defs;
    {
      std::vector<Variable> vec;
      vec.push_back(Variable("AVI","avi"));
      vec.push_back(Variable("BAHRA","bahra"));
      vec.push_back(Variable("LOWER","10"));
      vec.push_back(Variable("PATH","/fred/bill/joe"));
      vec.push_back(Variable("fred","%bill%"));
      vec.push_back(Variable("bill","%fred%"));
      vec.push_back(Variable("hello","%hello%"));
      vec.push_back(Variable("mary","%jane%"));
      vec.push_back(Variable("jane","10"));
      defs.set_server().add_or_update_user_variables(vec);
   }

   // See page 31, section 5.1 variable inheritance, of SMS users guide
   std::string cmd = "%AVI%-%BAHRA%-%LOWER%-%AVI%";
   string expected = "avi-bahra-10-avi";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%ECF_VERSION%";   expected = Version::raw();
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%PATH%"; expected = "/fred/bill/joe";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "/%AVI%/%BAHRA%/%LOWER%%PATH%"; expected = "/avi/bahra/10/fred/bill/joe";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%mary%"; expected = "10"; // double substitution
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%fred%"; expected = "%fred%"; // infinite substitution
   BOOST_CHECK_MESSAGE(!defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%hello%"; expected = "%hello%"; // infinite substitution
   BOOST_CHECK_MESSAGE(!defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = Ecf::MICRO(); expected = Ecf::MICRO();
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%PATH"; expected = "%PATH";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%"; expected = "%";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%ERROR%"; expected = "%ERROR%";
   BOOST_CHECK_MESSAGE(!defs.variableSubsitution(cmd)," substitution expected to fail since ERROR does not exist");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = ""; expected = "";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");


   // new rules
   // %<VAR>:substitute %
   // If we find VAR, then use it, else use substitute
   cmd = "%AVI:goblly gook%"; expected = "avi";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%PATH:goblly::: gook%"; expected = "/fred/bill/joe";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%LOWER:fred% %AVI:fred2%"; expected = "10 avi";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%LOWER:fred% %AVI:fred2"; expected = "10 %AVI:fred2";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%NULL:goblly gook%"; expected = "goblly gook";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%NULL::goblly gook%"; expected = ":goblly gook";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%NULL:%"; expected = "";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%:%"; expected = "";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE( test_defs_variable_substitution_double_micro )
{
   std::cout <<  "ANode:: ...test_defs_variable_substitution_double_micro\n";

   Defs defs;

   std::string cmd = "%%"; std::string expected = "%";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%"; expected = "%%";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%%"; expected = "%%";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%%%"; expected = "%%%";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "date +%%Y.%%m.%%d"; expected = "date +%Y.%m.%d";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "printf %%02d %HOUR:00%"; expected = "printf %02d 00";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   defs.set_server().add_or_update_user_variables("HOUR","hammer time");
   cmd = "printf %%02d %HOUR:00%"; expected = "printf %02d hammer time";
   BOOST_CHECK_MESSAGE(defs.variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_SUITE_END()
