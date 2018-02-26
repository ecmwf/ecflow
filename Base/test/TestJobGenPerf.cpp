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
#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Defs.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
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
// Timing for /var/tmp/ma0/DEFS/metabuilder.def
// First/base point: real:10.15  user: 5.58  sys: 1.62
// After ECFLOW-846: real: 4.46  user: 3.72  sys: 0.74  # Only open/close include file once
// After ECFLOW-864: real: 4.36  user: 3.68  sys: 0.68  # minimise stat calls
//
//     strace -c ./Base/bin/gcc-4.8/release/perf_job_gen ./metabuilder.def
//
//                            % time     seconds  usecs/call     calls    errors syscall
//                            ------ ----------- ----------- --------- --------- ----------------
//   Before ECFLOW-864:        22.77    0.001159           0    132329     50737 stat
//   After  ECFLOW-864:        21.35    0.001097           0    125644     50737 stat
//
//
// After ECFLOW-1244:
//  - cacheing of stat of include files
//  - re-arranging EcfFile data member for hotness:
//
//time Base/bin/gcc-5.3.0/release/perf_job_gen ./metabuilder.def : submitted 5808 out of 7941( fastest of 10 attempts)
// - ECFLOW-1244: real: 2.86s user: 2.42s sys: 0.42s
//
//perf stat -r 10 -d Base/bin/gcc-5.3.0/release/perf_job_gen ./metabuilder.def
//Performance counter stats for 'Base/bin/gcc-5.3.0/release/perf_job_gen ./metabuilder.def' (10 runs):
//
//      2894.687079      task-clock (msec)         #    0.926 CPUs utilized            ( +-  0.84% )
//               93      context-switches          #    0.032 K/sec                    ( +- 18.48% )
//                2      cpu-migrations            #    0.001 K/sec                    ( +- 30.23% )
//           11,493      page-faults               #    0.004 M/sec                    ( +-  0.02% )
//   10,888,902,061      cycles                    #    3.762 GHz                      ( +-  0.37% )  (37.49%)
//    4,739,862,119      stalled-cycles-frontend   #   43.53% frontend cycles idle     ( +-  0.89% )  (50.03%)
//  <not supported>      stalled-cycles-backend
//   13,925,593,267      instructions              #    1.28  insns per cycle
//                                                 #    0.34  stalled cycles per insn  ( +-  0.05% )  (62.57%)
//    3,802,491,790      branches                  # 1313.611 M/sec                    ( +-  0.04% )  (62.59%)
//      108,612,244      branch-misses             #    2.86% of all branches          ( +-  0.21% )  (62.62%)
//    3,430,787,436      L1-dcache-loads           # 1185.201 M/sec                    ( +-  0.07% )  (49.52%)
//      260,132,335      L1-dcache-load-misses     #    7.58% of all L1-dcache hits    ( +-  0.11% )  (24.98%)
//      101,447,778      LLC-loads                 #   35.046 M/sec                    ( +-  0.26% )  (24.98%)
//  <not supported>      LLC-load-misses
//
//      3.126218696 seconds time elapsed                                          ( +-  2.82% )


int main(int argc, char* argv[])
{
   if (argc != 2) {
      cout << "TestJobGenPerf.cpp --> " << argv[0] << "\n";
      cout << "Expect single argument which is path to a defs file\n";
      return 1;
   }

   // delete the log file if it exists.
   std::string log_path = File::test_data("Base/test/TestJobGenPerf.log","Base");
   fs::remove(log_path);
   std::string path = argv[1];

#ifdef DEBUG
   cout << "Loading file " << path << " log file " << log_path  << "\n";
#endif
   Defs defs;
   std::string errorMsg,warningMsg;
   if (!defs.restore(path,errorMsg,warningMsg)) {
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
//   exit(0);

   // Check number of tasks, if the submitted output below is too low
   std::vector<Task*> tasks;
   defs.getAllTasks(tasks);

//#ifdef DEBUG
//   cout << "Total number of tasks: " << tasks.size() << "\n";
//   cout << "begin-all\n";
//#endif

   defs.beginAll();

//#ifdef DEBUG
//   cout << "Free all dependencies, free suspended time and trigger dependencies\n";
//#endif

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

//   for(size_t i = 0; i < tasks.size(); i++) {
//      if (tasks[i]->state() != NState::SUBMITTED) {
//         cout << "task " << tasks[i]->absNodePath() << " state: " << NState::toString(tasks[i]->state()) << "\n";
//         Node* parent = tasks[i]->parent();
//         while (parent) {
//            cout << "node " << parent->absNodePath() << " state: " << NState::toString(parent->state()) << "\n";
//            parent = parent->parent();
//         }
//      }
//   }
   // fs::remove(log_path);
   return 0;
}
