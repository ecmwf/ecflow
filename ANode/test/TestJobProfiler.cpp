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
#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_job_profiler )
{
   cout << "ANode:: ...test_job_profiler\n";

   // delete the log file if it exists.
   std::string log_path = File::test_data("ANode/test/logfile.txt","ANode");
   fs::remove(log_path);
   BOOST_CHECK_MESSAGE(!fs::exists( log_path ), "log file " << log_path << " not deleted ");

   // Create a new log, file, we will look in here to see if job profiling is working
   Log::create(log_path);


   // SET ECF_HOME, re-use exist test of directory and scripts
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("suite");
      suite->addVariable( Variable( Str::ECF_INCLUDE(), File::test_data("ANode/test/data/includes","ANode") ) );
      suite->addVariable( Variable( "ECF_HOME",         File::test_data("ANode/test/data/SMSHOME","ANode") ) );
      suite->addVariable( Variable( "SLEEPTIME", "10" ) );
      family_ptr fam = suite->add_family( "family" );
      fam->add_task( "t1" );
      fam->add_task( "t2" );
      fam->add_task( "t3" );
   }
   // cerr << theDefs << "\n";

   // begin , will cause creation of generated variables. The generated variables
   // are use in client scripts and used to locate the ecf files
   theDefs.beginAll();

   // By setting submitJobsInterval to -1, we enable the jobs profiling testing
   // createJobs enables us to ensure job size is profiled
   // spawn jobs set to false, do not create a separate process to spawn jobs
   JobsParam jobParam(-1 /*submitJobsInterval*/, true /*createJobs*/, false/* spawn jobs */);
   Jobs job(&theDefs);
   bool ok = job.generate( jobParam );
   BOOST_CHECK_MESSAGE(ok,"generate failed: " << jobParam.getErrorMsg());

   // Check the log file, has the profiling
   //   ERR:[08:53:43 1.11.2014] Jobs::generate: job generation time(0 seconds) is greater than job submission interval of -1 seconds!!
   //   MSG:[08:53:43 1.11.2014] SUITE:/suite - 1s
   //   MSG:[08:53:43 1.11.2014]  FAMILY:/suite/family - 1s
   //   MSG:[08:53:43 1.11.2014]   TASK:/suite/family/t1 - job size:3186 - 1s
   //   MSG:[08:53:43 1.11.2014]   TASK:/suite/family/t2 - job size:2404 - 1s
   //   MSG:[08:53:43 1.11.2014]   TASK:/suite/family/t3 - job size:2539 - 1s

   std::string log_file_contents;
   BOOST_CHECK_MESSAGE(File::open(log_path,log_file_contents), "Could not open log file at " << log_path);
   BOOST_CHECK_MESSAGE(!log_file_contents.empty(),"log file is is empty ?");
   BOOST_CHECK_MESSAGE(log_file_contents.find("SUITE:/suite") != std::string::npos,        "SUITE:/suite  not in profile");
   BOOST_CHECK_MESSAGE(log_file_contents.find("FAMILY:/suite/family") != std::string::npos,"FAMILY:/suite/family  not in profile");
   BOOST_CHECK_MESSAGE(log_file_contents.find("TASK:/suite/family/t1") != std::string::npos,"TASK:/suite/family/t1  not in profile");
   BOOST_CHECK_MESSAGE(log_file_contents.find("TASK:/suite/family/t2") != std::string::npos,"TASK:/suite/family/t2  not in profile");
   BOOST_CHECK_MESSAGE(log_file_contents.find("TASK:/suite/family/t3") != std::string::npos,"TASK:/suite/family/t3  not in profile");
   BOOST_CHECK_MESSAGE(log_file_contents.find("job size") != std::string::npos,             "job size  not in profile");

   // Remove the log file. Comment out for debugging
   fs::remove(log_path);

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
