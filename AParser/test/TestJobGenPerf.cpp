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

#include "Defs.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "DefsStructureParser.hpp"
#include "JobProfiler.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;


// This relies on Pyext/samples/TestJobGenPerf.py to make any defs amenable
// for this test program.
//
// The defs is in /var/tmp/ma0/ECFLOW_TEST/TestJobGenPerf
//
int main(int argc, char* argv[])
{
   if (argc != 2) {
      cout << "Expect single argument which is path to a defs file\n";
      return 1;
   }

   // delete the log file if it exists.
   std::string log_path = File::test_data("AParser/test/TestJobGenPerf.log","AParser");
   fs::remove(log_path);


   std::string path = argv[1];

   Defs defs;
   DefsStructureParser checkPtParser( &defs, path);
   std::string errorMsg,warningMsg;
   if (!checkPtParser.doParse(errorMsg,warningMsg)) {
      cout << errorMsg << "\n";
      cout << warningMsg << "\n";
      return 1;
   }

   // Check number of tasks, if the submitted output below is too low
   // Then remove, limits,triggers,time,cron so more jobs can be generated.
   std::vector<Task*> tasks;
   defs.getAllTasks(tasks);
   cout << "Tasks = " << tasks.size() << "\n";

   defs.beginAll();


   // Create a new log, file, place after begin to avoid queued state
   Log::create(log_path);

   // This controls the log output when job generation > submitJobsInterval
   JobProfiler::set_suite_threshold(1500);
   JobProfiler::set_family_threshold(1000);
   JobProfiler::set_task_threshold(0);

   JobsParam jobParam(20 /*submitJobsInterval*/, true /*createJobs*/, false/* spawn jobs */);
   Jobs job(&defs);
   bool ok = job.generate( jobParam );
   if (!ok) cout << " generate failed: " << jobParam.getErrorMsg();
   cout << "submitted " << jobParam.submitted().size() << "\n";

   // fs::remove(log_path);

   return 0;
}
