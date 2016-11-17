/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
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
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Version.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_variable_substitution )
{
    std::cout <<  "ANode:: ...test_variable_substitution\n";

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    {
		s->addVariable(Variable("AVI","avi"));
		s->addVariable(Variable("BAHRA","bahra"));
		s->addVariable(Variable("LOWER","10"));
		s->addVariable(Variable("PATH","/fred/bill/joe"));
		s->addVariable(Variable("EMPTY_VARIABLE",""));
		s->addVariable(Variable("fred","%bill%"));
		s->addVariable(Variable("bill","%fred%"));
		s->addVariable(Variable("hello","%hello%"));
		s->addVariable(Variable("mary","%jane%"));
		s->addVariable(Variable("jane","10"));
	}


 	// See page 31, section 5.1 variable inheritance, of SMS users guide
 	std::string cmd = "%AVI%-%BAHRA%-%LOWER%-%AVI%";
	string expected = "avi-bahra-10-avi";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd),"substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%ECF_VERSION%";
   expected = Version::raw();
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%PATH%"; expected = "/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "/%AVI%/%BAHRA%/%LOWER%%PATH%"; expected = "/avi/bahra/10/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%EMPTY_VARIABLE%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%mary%"; expected = "10"; // double substitution
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%fred%"; expected = "%fred%"; // infinite substitution
 	BOOST_CHECK_MESSAGE(!s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%hello%"; expected = "%hello%"; // infinite substitution
 	BOOST_CHECK_MESSAGE(!s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = Ecf::MICRO(); expected = Ecf::MICRO();
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%PATH"; expected = "%PATH";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%%"; expected = "%";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%ERROR%"; expected = "%ERROR%";
 	BOOST_CHECK_MESSAGE(!s->variableSubsitution(cmd)," substitution expected to fail since ERROR does not exist");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = ""; expected = "";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");


 	// new rules
 	// %<VAR>:substitute %
 	// If we find VAR, then use it, else use substitute
 	cmd = "%AVI:goblly gook%"; expected = "avi";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%PATH:goblly::: gook%"; expected = "/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%LOWER:fred% %AVI:fred2%"; expected = "10 avi";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%LOWER:fred% %AVI:fred2"; expected = "10 %AVI:fred2";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%EMPTY_VARIABLE::goblly gook%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%NULL:goblly gook%"; expected = "goblly gook";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%NULL::goblly gook%"; expected = ":goblly gook";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%NULL:%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%:%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE( test_variable_substitution_double_micro )
{
   std::cout <<  "ANode:: ...test_variable_substitution_double_micro\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite");

   std::string cmd = "%%"; std::string expected = "%";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%"; expected = "%%";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%%"; expected = "%%";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%%%%%"; expected = "%%%";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "date +%%Y.%%m.%%d"; expected = "date +%Y.%m.%d";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "printf %%02d %HOUR:00%"; expected = "printf %02d 00";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   s->addVariable(Variable("HOUR","hammer time"));
   cmd = "printf %%02d %HOUR:00%"; expected = "printf %02d hammer time";
   BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd),"substitution failed");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE( test_user_variable_substitution )
{
    std::cout <<  "ANode:: ...test_user_variable_substitution\n";

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    {
		s->addVariable(Variable("AVI","avi"));
		s->addVariable(Variable("BAHRA","bahra"));
		s->addVariable(Variable("LOWER","10"));
		s->addVariable(Variable("PATH","/fred/bill/joe"));
		s->addVariable(Variable("EMPTY_VARIABLE",""));
		s->addVariable(Variable("fred","%bill%"));
		s->addVariable(Variable("bill","%fred%"));
		s->addVariable(Variable("hello","%hello%"));
		s->addVariable(Variable("mary","%jane%"));
		s->addVariable(Variable("jane","10"));
	}

    NameValueMap user_variables;
    user_variables.insert( std::make_pair(string("AVI"),string("_avi")) );
    user_variables.insert( std::make_pair(string("BAHRA"),string("_bahra")) );
    user_variables.insert( std::make_pair(string("LOWER"),string("_10")) );
    user_variables.insert( std::make_pair(string("PATH"),string("_/fred/bill/joe")) );
    user_variables.insert( std::make_pair(string("EMPTY_VARIABLE"),string("_")) );
    user_variables.insert( std::make_pair(string("fred"),string("%bill%")) );
    user_variables.insert( std::make_pair(string("bill"),string("%fred%")) );
    user_variables.insert( std::make_pair(string("hello"),string("%hello%")) );
    user_variables.insert( std::make_pair(string("mary"),string("%jane%")) );
    user_variables.insert( std::make_pair(string("jane"),string("_10")) );

 	// See page 31, section 5.1 variable inheritance, of SMS users guide
 	std::string cmd = "%AVI%-%BAHRA%-%LOWER%-%AVI%";   string expected = "_avi-_bahra-_10-_avi";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables),"substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%PATH%"; expected = "_/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "/%AVI%/%BAHRA%/%LOWER%%PATH%"; expected = "/_avi/_bahra/_10_/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%EMPTY_VARIABLE%"; expected = "_";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%mary%"; expected = "_10"; // double substitution
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%fred%"; expected = "%fred%"; // infinite substitution
 	BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%hello%"; expected = "%hello%"; // infinite substitution
 	BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = Ecf::MICRO(); expected = Ecf::MICRO();
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%PATH"; expected = "%PATH";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");


 	cmd = "%%"; expected = "%";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%ERROR%"; expected = "%ERROR%";
 	BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd,user_variables)," substitution expected to fail since ERROR does not exist");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = ""; expected = "";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");


 	// new rules
 	// %<VAR>:substitute %
 	// If we find VAR, then use it, else use substitute
 	cmd = "%AVI:goblly gook%"; expected = "_avi";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%PATH:goblly::: gook%"; expected = "_/fred/bill/joe";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%LOWER:fred% %AVI:fred2%"; expected = "_10 _avi";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%LOWER:fred% %AVI:fred2"; expected = "_10 %AVI:fred2";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%EMPTY_VARIABLE::goblly gook%"; expected = "_";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

 	cmd = "%NULL:goblly gook%"; expected = "goblly gook";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%NULL::goblly gook%"; expected = ":goblly gook";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%NULL:%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

	cmd = "%:%"; expected = "";
 	BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
 	BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}


BOOST_AUTO_TEST_CASE( test_user_variable_substitution_1 )
{
    std::cout <<  "ANode:: ...test_user_variable_substitution_1\n";

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable("AVI","avi"));

    NameValueMap user_variables;
    user_variables.insert( std::make_pair(string("AVI:goblly gook"),string("avtar")) );

   // new rules
   // %<VAR>:substitute %
   // If we find VAR, then use it, else use substitute
   // However when we have user_variables if we find the complete string
   // in the user variable list, we use user veriable value:
   // cmd = %FRED:BILL%"   and   user_variable = "FRED:BILL","Joe90"   ===> cmd = "Joe90"
   std::string cmd = "%AVI:goblly gook%"; std::string expected = "avtar";
   BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%AVI:goblly%"; expected = "avi";
   BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

   cmd = "%FRED:goblly%"; expected = "goblly";
   BOOST_CHECK_MESSAGE(s->variable_substitution(cmd,user_variables)," substitution failed ");
   BOOST_CHECK_MESSAGE( cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}


static std::vector<std::string> required_server_variables()
{
   std::vector<std::string> required_server_variables;
   required_server_variables.push_back( Str::ECF_PORT() );
   required_server_variables.push_back( Str::ECF_NODE() );
   required_server_variables.push_back( Str::ECF_HOST() );

   required_server_variables.push_back( Str::ECF_HOME() );
   required_server_variables.push_back( std::string("ECF_LOG") );
   required_server_variables.push_back( std::string("ECF_CHECK") );
   required_server_variables.push_back( std::string("ECF_CHECKOLD") );

   // These variable are read in from the environment, but are not exposed
   // since they only affect the server
   // ECF_CHECKINTERVAL
   // ECF_LISTS

   // variables that can be overridden, in the suite definition
   required_server_variables.push_back( std::string("ECF_JOB_CMD") );
   required_server_variables.push_back( std::string("ECF_KILL_CMD") );
   required_server_variables.push_back( std::string("ECF_STATUS_CMD") );
   required_server_variables.push_back( std::string("ECF_URL_CMD") );
   required_server_variables.push_back( std::string("ECF_URL_BASE") );
   required_server_variables.push_back( std::string("ECF_URL") );
   required_server_variables.push_back( std::string("ECF_MICRO") );

   // Reference variable, these should be read only
   required_server_variables.push_back( std::string("ECF_PID") );   // server PID
   required_server_variables.push_back( std::string("ECF_VERSION") );// server version
   return required_server_variables;
}

BOOST_AUTO_TEST_CASE( test_server_variable_substitution )
{
   std::cout <<  "ANode:: ...test_server_variable_substitution\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite");

   std::vector<std::string> vec = required_server_variables();
   for(size_t i = 0; i < vec.size(); i++) {
      if (vec[i] == "ECF_PID") continue;         // CANT test since, this is process ID of server
      std::string value;
      BOOST_CHECK_MESSAGE(s->findParentVariableValue(vec[i],value),"Could not find Server variable " << vec[i]);
      BOOST_CHECK_MESSAGE(!value.empty(),"Empty server variable value for " << vec[i]);
   }

   for(size_t i = 0; i < vec.size(); i++) {
      if (vec[i] == "ECF_JOB_CMD") continue;     // CANT test since it requires %ECF_JOB% and %ECF_JOBOUT%
      if (vec[i] == "ECF_KILL_CMD") continue;    // CANT test since it requires %ECF_PID%
      if (vec[i] == "ECF_STATUS_CMD") continue;  // CANT test since it requires %ECF_RID%
      if (vec[i] == "ECF_PID") continue;         // CANT test since, this is process ID of server
      std::string cmd = "%";
      cmd += vec[i];
      cmd += "%";
      BOOST_CHECK_MESSAGE(s->variableSubsitution(cmd)," substitution failed for " << vec[i] << " : " << cmd);
      if (vec[i] == "ECF_VERSION") {
         BOOST_CHECK_MESSAGE( cmd == Version::raw(), "expected '" << Version::raw() << "' but found '" << cmd << "'");
      }
   }
}

BOOST_AUTO_TEST_CASE( test_generated_variable_substitution_of_ECF_OUT )
{
   // test that if ECF_OUT is defined using %, then we perform variable substitution
   std::cout <<  "ANode:: ...test_generated_variable_substitution_of_ECF_OUT\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite");
   s->addVariable(Variable("PATH","/fred/bill/joe"));
   s->addVariable(Variable("ECF_HOME","/ecf_home"));
   family_ptr f = s->add_family("f");
   task_ptr t = f->add_task("t");
   t->addVariable(Variable("ECF_OUT","%PATH%"));
   family_ptr f1 = s->add_family("f1");
   f1->addVariable(Variable("PATH2","/fred/bill/joe2"));
   task_ptr t1 = f1->add_task("t1");
   t1->addVariable(Variable("ECF_OUT","%PATH2%"));

   // begin_all
   defs.beginAll();
   t->update_generated_variables();
   t1->update_generated_variables();

   // cout << defs;

   string value;
   value.clear();
   t->findParentVariableValue(Str::ECF_JOBOUT(),value);
   BOOST_CHECK_MESSAGE(value == "/fred/bill/joe/suite/f/t.0","ECF_JOBOUT expected /fred/bill/joe/suite/f/t.0, but found " << value);

   value.clear();
   t1->findParentVariableValue(Str::ECF_JOBOUT(),value);
   BOOST_CHECK_MESSAGE(value == "/fred/bill/joe2/suite/f1/t1.0","ECF_JOBOUT expected /fred/bill/joe/suite/f/t.0, but found " << value);
}

BOOST_AUTO_TEST_SUITE_END()

