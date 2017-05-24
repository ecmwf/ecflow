//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$
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
#include <string>
#include <iostream>
#include <fstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <boost/timer.hpp>
#include "boost/date_time/posix_time/posix_time_types.hpp"

#include "DefsStructureParser.hpp"
#include "Defs.hpp"
#include "NodeContainer.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "PrintStyle.hpp"
#include "PersistHelper.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "DurationTimer.hpp"
#include "Log.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

// This test is used to find a task given a path of the form:
//      suite/family/task
//    suite/family/family/task
void test_find_task_using_path( NodeContainer* f,const Defs& defs )
{
   if (f != defs.findAbsNode(f->absNodePath()).get() ) cout << "Could not find path " << f->absNodePath() << "\n";

   BOOST_FOREACH(node_ptr t, f->nodeVec()) {
      if (t.get() != defs.findAbsNode(t->absNodePath()).get()) cout <<  "Could not find path " << t->absNodePath() << "\n";
      Family* family = t->isFamily();
      if (family) {
         test_find_task_using_path(family, defs);
      }
   }
}

// Create derived class, so that we can time the parse only
// i.e ignore expression build/checking and limit checking
class TestDefsStructureParser : public DefsStructureParser {
public:
   TestDefsStructureParser(Defs* defsfile, const std::string& file_name) : DefsStructureParser(defsfile,file_name) {}
   bool do_parse_only(std::string& errorMsg) { return DefsStructureParser::do_parse_only(errorMsg); }
};


int main(int argc, char* argv[])
{
//   cout << "argc = " << argc << "\n";
//   for(int i = 0; i < argc; i++) {
//      cout << "arg " << i << ":" << argv[i] << "\n";
//   }

   if (argc != 2) {
      cout << "Expect single argument which is path to a defs file\n";
      return 1;
   }

   std::string path = argv[1];

   DurationTimer duration_timer;
   boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed

   /// If this is moved below, we get some caching affect, with the persist and reload timing
   Defs defs;
   {
      timer.restart();
      std::string errorMsg,warningMsg;
      bool result = defs.restore(path,errorMsg,warningMsg);
      std::cout << " Parsing Node tree and AST creation time = " << timer.elapsed() << " parse(" << result << ")" << endl;
   }

   {
      Defs local_defs;
      timer.restart();
      TestDefsStructureParser checkPtParser( &local_defs, path);
      std::string errorMsg;
      bool result = checkPtParser.do_parse_only(errorMsg);
      std::cout << " Parsing Node tree *only* time           = " << timer.elapsed() << " parse(" << result << ")" << endl;
   }

   {
      // Test time for persisting to defs file only
      std::string tmpFilename = "tmp.def";

      timer.restart();
      defs.save_as_checkpt(tmpFilename);
      cout << " Save as DEFS checkpoint, time taken                                   = " << timer.elapsed()  << endl;

      std::remove(tmpFilename.c_str());
   }
   {
      // Test time for persisting to BOOST checkpoint file only
      std::string tmpFilename = "tmp.def";

      timer.restart();
      defs.boost_save_as_checkpt(tmpFilename);
      cout << " Save as BOOST checkpoint, time taken                                  = " << timer.elapsed()  << endl;

      std::remove(tmpFilename.c_str());
   }

   {
      // may need to comment out output for large differences. Will double the time.
      timer.restart();
      PersistHelper helper;
      bool result = helper.test_persist_and_reload(defs,PrintStyle::MIGRATE);
      cout << " Checkpt(DEFS) and reload, time taken            = "
            << timer.elapsed()  << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
   }

#if defined(BINARY_ARCHIVE)
   {
      bool do_compare = false;
      timer.restart();
      PersistHelper helper;
      bool result = helper.test_boost_checkpt_and_reload(defs, do_compare,ecf::Archive::BINARY);
      cout << " Checkpt(BINARY_ARCHIVE) and reload , time taken   = ";
      cout << timer.elapsed() << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
   }
#elif defined(PORTABLE_BINARY_ARCHIVE)
   {
      bool do_compare = false;
      timer.restart();
      PersistHelper helper;
      bool result = helper.test_boost_checkpt_and_reload(defs, do_compare, ecf::Archive::PORTABLE_BINARY);
      cout << " Checkpt(PORTABLE_BINARY_ARCHIVE) and reload , time taken   = ";
      cout << timer.elapsed() << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
   }
#elif defined(EOS_PORTABLE_BINARY_ARCHIVE)
   {
      bool do_compare = false;
      timer.restart();
      PersistHelper helper;
      bool result = helper.test_boost_checkpt_and_reload(defs, do_compare, ecf::Archive::EOS_PORTABLE_BINARY);
      cout << " Checkpt(EOS_PORTABLE_BINARY_ARCHIVE) and reload , time taken   = ";
      cout << timer.elapsed() << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
   }
#else
   {
      bool do_compare = false;
      timer.restart();
      PersistHelper helper;
      bool result = helper.test_boost_checkpt_and_reload(defs, do_compare, ecf::Archive::TEXT);
      cout << " Checkpt(TEXT_ARCHIVE) and reload , time taken   = ";
      cout << timer.elapsed() << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
   }
//   {
//      bool do_compare = false;
//      timer.restart();
//      PersistHelper helper;
//      for(int i = 0; i < 5; i++) {
//         bool result = helper.test_boost_checkpt_and_reload(defs, do_compare, ecf::Archive::TEXT);
//         if (!helper.errorMsg().empty())  cout << " result(" << result << ") msg(" << helper.errorMsg() << ")\n";
//      }
//      cout << " Checkpt(TEXT_ARCHIVE) and reload 5 times: Average time taken = ";
//      cout << timer.elapsed()/5 << " : file_size(" << helper.file_size() << ")\n";
//   }
#endif

   {
      timer.restart();
      BOOST_FOREACH(suite_ptr s, defs.suiteVec()) { test_find_task_using_path(s.get(),defs); }
      cout << " Test all paths can be found. time taken = " << timer.elapsed() << endl;
   }
   {
      // Time how long it takes for job submission. Must call begin on all suites first.
      timer.restart();
      defs.beginAll();
      int count = 10;
      JobsParam jobsParam; // default is not to create jobs, hence only used in testing
      Jobs jobs(&defs);
      for (int i = 0; i < count; i++) {jobs.generate(jobsParam);}
      cout << " time for " << count << " jobSubmissions          = " << timer.elapsed() << "s jobs:" << jobsParam.submitted().size() << endl;
   }
   {
      // Time how long it takes for post process
      timer.restart();
      string errorMsg,warningMsg;
      bool result = defs.check(errorMsg,warningMsg);
      cout << " Time for Defs::check (  inlimit resolution)      = " << timer.elapsed() <<  " result(" << result << ")" << endl;
   }

   {
      // Time how long it takes to delete all nodes/ references. Delete all tasks and then suites/families.
      timer.restart();
      std::vector<Task*> tasks;
      defs.getAllTasks(tasks);
      BOOST_FOREACH(Task* t, tasks) {
         if (!defs.deleteChild(t)) cout << "Failed to delete task\n";
      }
      tasks.clear(); defs.getAllTasks(tasks);
      if (!tasks.empty()) cout << "Expected all tasks to be deleted but found " << tasks.size() << "\n";

      std::vector<suite_ptr> vec = defs.suiteVec(); // make a copy, to avoid invalidating iterators
      BOOST_FOREACH(suite_ptr s, vec) {
         std::vector<node_ptr> familyVec = s->nodeVec(); // make a copy, to avoid invalidating iterators
         BOOST_FOREACH(node_ptr f, familyVec) {
            if (!defs.deleteChild(f.get())) cout << "Failed to delete family\n";
         }
         if (!s->nodeVec().empty()) cout << "Expected all Families to be deleted but found " << s->nodeVec().size() << "\n";
         if (!defs.deleteChild(s.get())) cout << "Failed to delete suite\n";
      }
      if (!defs.suiteVec().empty()) cout << "Expected all Suites to be deleted but found " << defs.suiteVec().size() << "\n";

      cout << " time for deleting all nodes                   = " << timer.elapsed() << endl;
   }
   cout << " Total elapsed time = " << duration_timer.duration() << " seconds\n";
}
