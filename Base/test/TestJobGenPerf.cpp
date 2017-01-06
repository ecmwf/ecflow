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
#include <iostream>
#include <string>
#include <vector>

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

//#define DEBUG 1

// This relies on Pyext/samples/TestJobGenPerf.py to make any defs amenable
// for this test program.
//
// The defs is in /var/tmp/ma0/ECFLOW_TEST/TestJobGenPerf
//
int main(int argc, char* argv[])
{
   if (argc != 2) {
      cout << "TestJobGenPerf.cpp --> " << argv[0] << "\n";
      cout << "Expect single argument which is path to a defs file\n";
      return 1;
   }

   // delete the log file if it exists.
   std::string log_path = File::test_data("Base/test/TestJobGenPerf.log","AParser");
   fs::remove(log_path);
   std::string path = argv[1];

#ifdef DEBUG
   cout << "Loading file " << path << " log file " << log_path  << "\n";
#endif
   Defs defs;
   DefsStructureParser checkPtParser( &defs, path);
   std::string errorMsg,warningMsg;
   if (!checkPtParser.doParse(errorMsg,warningMsg)) {
      cout << errorMsg << "\n";
      cout << warningMsg << "\n";
      return 1;
   }

#ifdef DEBUG
   cout << "remove dodgy suites, these are based on localhost\n";
#endif
//   std::vector<std::string> suites_to_remove;
//   suites_to_remove.push_back("codes_ui");
//   suites_to_remove.push_back("libemos_test");
//   suites_to_remove.push_back("metview");
//   suites_to_remove.push_back("ecflow");
//   suites_to_remove.push_back("mir_bundle");
//   for(size_t i = 0; i < suites_to_remove.size(); ++i) {
//      suite_ptr suite = defs.findSuite(suites_to_remove[i]);
//      if (suite) suite->remove();
//   }
//   cout << defs ;

   // Check number of tasks, if the submitted output below is too low
   std::vector<Task*> tasks;
   defs.getAllTasks(tasks);

#ifdef DEBUG
   cout << "Total number of tasks" << tasks.size() << "\n";
   cout << "begin-all\n";
#endif
   defs.beginAll();

#ifdef DEBUG
   cout << "Free all dependencies, free suspended time and trigger dependencies\n";
#endif

   std::vector<node_ptr> all_nodes;
   defs.get_all_nodes(all_nodes);
   for(size_t i = 0; i < all_nodes.size(); ++i) {
      if (all_nodes[i]->isSuspended())  all_nodes[i]->resume();
      all_nodes[i]->freeTrigger();
      all_nodes[i]->freeHoldingDateDependencies();
      all_nodes[i]->freeHoldingTimeDependencies();
      if (all_nodes[i]->state() == NState::COMPLETE && all_nodes[i]->isTask()) {
         all_nodes[i]->set_state(NState::QUEUED);
      }
   }


   // Create a new log, file, place after begin to avoid queued state
   Log::create(log_path);

   // This controls the log output when job generation > submitJobsInterval
   JobProfiler::set_task_threshold(5); // 5ms 1000 is one second

   JobsParam jobParam(20 /*submitJobsInterval*/, true /*createJobs*/, false/* spawn jobs */);
   Jobs job(&defs);
   if (!job.generate( jobParam )) cout << " generate failed: " << jobParam.getErrorMsg();
   cout << "submitted " << jobParam.submitted().size() << " out of " << tasks.size() << "\n";

   // fs::remove(log_path);
   return 0;
}
