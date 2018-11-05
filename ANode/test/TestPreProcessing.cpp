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
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <set>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "JobsParam.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

void findVariable(std::string& line, std::set<std::string>& variables)
{
	// scan for variables
 	// edit SMSFETCH "/home/ma/map/sms/smsfectch -F %ECF_FILES% -I %ECF_INCLUDE%"
	// We can also have
	//
	// "%<VAR>:<substitute>% i.e if VAR exist use it, else use substitute
	//
	// i.e VAR is defined as BILL
	//  %VAR:fred --f%  will either be "BILL" or if VAR is not defined "fred --f"
 	while ( true ) {
		size_t firstPercentPos = line.find( Ecf::MICRO() );
		if ( firstPercentPos == string::npos ) break;
		size_t secondPercentPos = line.find( Ecf::MICRO(), firstPercentPos + 1 );
		if ( secondPercentPos == string::npos ) break;
		if ( secondPercentPos - firstPercentPos <= 1 ) break; // handle %% with no characters in between

		string percentVar( line.begin() + firstPercentPos+1, line.begin() + secondPercentPos );

 		size_t firstColon = percentVar.find( ':' );
		if ( firstColon != string::npos ) {
			string var( percentVar.begin(), percentVar.begin() + firstColon );
			percentVar = var;
 		}

		// Ignore auto-generated variables
		if (	percentVar.find("ECF_") == string::npos &&
				percentVar != "DATE"  &&
				percentVar != "DAY" &&
				percentVar != "DD" &&
				percentVar != "DOW" &&
				percentVar != "DOY" &&
				percentVar != "MM" &&
				percentVar != "MONTH" &&
				percentVar != "YYYY" &&
				percentVar != "TASK" &&
				percentVar != "FAMILY" &&
				percentVar != "FAMILY1" &&
				percentVar != "SUITE"
			) {
			variables.insert( percentVar );
		}

		//std::cerr << "line before delete " << line << "\n";
		line.erase(firstPercentPos, secondPercentPos - firstPercentPos + 1);
		//std::cerr << "line after delete " << line << "\n";
	}
}

void autoDiscoverVariables(const std::string& directory, std::set<std::string>& variables)
{
	fs::path full_path( fs::initial_path<fs::path>() );
	full_path = fs::system_complete( fs::path( directory ) );

	BOOST_CHECK(fs::exists( full_path ));
	BOOST_CHECK(fs::is_directory( full_path ));

	//std::cout << "\nIn directory: " << full_path << "\n\n";
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr ) {
		try {
         fs::path relPath(directory + "/" + dir_itr->path().filename().string());

 			// recurse down directories
		    if ( fs::is_directory(dir_itr->status()) )  {
		    	autoDiscoverVariables(relPath.string(),variables);
		    	continue;
		    }
			// std::cout << "......autoDiscoverVariables for file " << relPath.string() << "\n";
		    if (relPath.extension() != ".h")  continue; // Only look at .h files

		    // open the file, and find variables.
		    std::vector<std::string> lines;
		 	if ( File::splitFileIntoLines(relPath.string(), lines) ) {
		 		for(auto & line : lines) {
		 			findVariable(line, variables);
		 		}
		 	}
		}
		catch ( const std::exception & ex ) {
 			std::cout << "Exception::" << dir_itr->path().filename() << " " << ex.what() << std::endl;
		}
	}
}


BOOST_AUTO_TEST_SUITE( NodeTestSuite )

// *Auto* discover the good/bad sms files
void test_sms_preprocessing(const std::string& directory, bool pass)
{
	// SET ECF_HOME
	std::string ecf_home = directory;

	fs::path full_path( fs::initial_path<fs::path>() );
	full_path = fs::system_complete( fs::path( directory ) );
	BOOST_CHECK(fs::exists( full_path ));
	BOOST_CHECK(fs::is_directory( full_path ));

	// Create a defs file, where the task name mirrors the sms files in the given directory
 	Defs theDefs;
 	{
      suite_ptr suite = theDefs.add_suite("suite");
		suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
		suite->addVariable( Variable( Str::ECF_OUT(),     "$ECF_HOME" ) );
		suite->addVariable( Variable( "SLEEPTIME", "10" ) );
      family_ptr fam = suite->add_family( "family" );

		// for operations auto discover the variables used in the header files and give
		// them a dummy value. This would allow the test to pass when doing
		// variable substitution. hence if variable substitution fails its likely to be
		// a bug in autoDiscoverVariables
		std::set<std::string> discoveredVariables;
		autoDiscoverVariables(ecf_home + "/includes", discoveredVariables );
		BOOST_FOREACH(const string& var, discoveredVariables) {
// 			cerr << "autoDiscoverVariables = " << var << "\n";
			suite->addVariable( Variable( var, "gobblygook" ) );
		}

		//std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
		fs::directory_iterator end_iter;
		for (fs::directory_iterator dir_itr( full_path ); dir_itr != end_iter; ++dir_itr) {
			try {
				fs::path relPath( directory + "/" + dir_itr->path().filename().string());

				// Ignore directories were only interested in .ecf files.
 				if (fs::is_directory(relPath)) continue;
            if (File::getExt(relPath.filename().string()) != "ecf" ) continue; // ignore other files

				//std::cout << "......Parsing file " << relPath.string() << "\n";
 				//std::cout << "      adding task name " << relPath.leaf() << "\n";
 				fam->add_task( relPath.stem().string() );
			}
			catch ( const std::exception & ex ) {
				std::cout << "Exception " << dir_itr->path().filename() << " " << ex.what() << std::endl;
			}
		}
		//cerr << "The defs\n" << theDefs << "\n";
	}

	// get all the task
	std::vector<Task*> theTasks;
 	theDefs.getAllTasks(theTasks);

	// Override ECF_HOME.   ECF_HOME is need to locate the ecf files
	theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),ecf_home);

	/// begin , will cause creation of generated variables. The generated variables
	/// are used in client scripts(sms) and used to locate the sms files
	theDefs.beginAll();

	// Test Job creator, this will pre-process and perform variable substitution on ecf files
	BOOST_FOREACH(Task* t, theTasks) {

	   //cout << "  task " << t->absNodePath() << "\n";
	   JobsParam jobsParam; // create jobs =  false, spawn_jobs = false
	   bool ok = t-> submitJob( jobsParam ) ;

	   if ( pass ) { // Test expected to pass
	      BOOST_CHECK_MESSAGE(ok,"Failed to create jobs. " << jobsParam.getErrorMsg() );
	   }
	   else {        // test expected to fail
	      BOOST_CHECK_MESSAGE(!ok,"Expected failure " << jobsParam.getErrorMsg() );
	      BOOST_CHECK_MESSAGE(!ok,"expected no passes but found " <<  t->absNodePath() << " passes");
	      //cerr << "\n" << jobsParam.getErrorMsg() << " \n"; // un-comment to ensure correct error message
	   }
	}
}


BOOST_AUTO_TEST_CASE( test_good_sms )
{
	cout << "ANode:: ...test_good_ecf\n";

   std::string path = File::test_data("ANode/test/data/SMSHOME2/good","ANode");

	// All the sms in this directory are expected to pass
	test_sms_preprocessing(path, true);
}

BOOST_AUTO_TEST_CASE( test_bad_sms )
{
	cout << "ANode:: ...test_bad_ecf\n";

   std::string path = File::test_data("ANode/test/data/SMSHOME2/bad","ANode");

	// All the sms in this directory are expected to fail
	test_sms_preprocessing(path, false);

	/// Destroy System singleton to avoid valgrind from complaining
	System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
