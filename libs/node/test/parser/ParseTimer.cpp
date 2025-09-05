/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <iostream>
#include <string>

#include "PersistHelper.hpp"
#include "TemporaryFile.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

using namespace std;
using namespace ecf;

// This test is used to find a task given a path of the form:
//      suite/family/task
//    suite/family/family/task
void test_find_task_using_path(NodeContainer* f, const Defs& defs) {
    if (f != defs.findAbsNode(f->absNodePath()).get()) {
        cout << "Could not find path " << f->absNodePath() << "\n";
    }

    for (node_ptr t : f->nodeVec()) {
        if (t.get() != defs.findAbsNode(t->absNodePath()).get()) {
            cout << "Could not find path " << t->absNodePath() << "\n";
        }
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
    TestDefsStructureParser(Defs* defsfile, const std::string& file_name) : DefsStructureParser(defsfile, file_name) {}
    bool do_parse_file(std::string& errorMsg) { return DefsStructureParser::do_parse_file(errorMsg); }
};

int main(int argc, char* argv[]) {
    //   cout << "argc = " << argc << "\n";
    //   for(int i = 0; i < argc; i++) {
    //      cout << "arg " << i << ":" << argv[i] << "\n";
    //   }

    if (argc != 2) {
        cout << "Expect single argument which is path to a defs file\n";
        return 1;
    }

    std::string path = argv[1];

    ScopedPerformanceTimer scope;
    PerformanceTimer timer;

    /// If this is moved below, we get some caching affect, with the persist and reload timing
    Defs defs;
    {
        timer.start();
        std::string errorMsg, warningMsg;
        bool result = defs.restore(path, errorMsg, warningMsg);
        cout << " Parsing Node tree & AST creation time parse(" << result << ") = " << timer << endl;
    }
    {
        Defs local_defs;
        timer.start();
        TestDefsStructureParser checkPtParser(&local_defs, path);
        std::string errorMsg;
        bool result = checkPtParser.do_parse_file(errorMsg);
        cout << " Parsing Node tree *only* time         parse(" << result << ") = " << timer << endl;
    }
    {
        timer.start();
        std::string defs_as_string;
        defs.write_to_string(defs_as_string, PrintStyle::DEFS);
        Defs newDefs;
        std::string error_msg, warning_msg; // ignore error since some input defs have invalid triggers
        newDefs.restore_from_string(defs_as_string, error_msg, warning_msg);
        cout << " Save and restore as string(DEFS)               = " << timer << " -> string size("
             << defs_as_string.size() << ")" << endl;
    }
    {
        timer.start();
        std::string defs_as_string;
        defs.write_to_string(defs_as_string, PrintStyle::NET);
        Defs newDefs;
        std::string error_msg, warning_msg; // ignore error since some input defs have invalid triggers
        newDefs.restore_from_string(defs_as_string, error_msg, warning_msg);
        cout << " Save and restore as string(NET)                = " << timer << " -> string size("
             << defs_as_string.size() << ") checks relaxed" << endl;
    }
    {
        timer.start();
        std::string defs_as_string;
        defs.write_to_string(defs_as_string, PrintStyle::MIGRATE);
        Defs newDefs;
        std::string error_msg, warning_msg; // ignore error since some input defs have invalid triggers
        newDefs.restore_from_string(defs_as_string, error_msg, warning_msg);
        cout << " Save and restore as string(MIGRATE)            = " << timer << " -> string size("
             << defs_as_string.size() << ")" << endl;
    }
    {
        // Test time for persisting to defs file only
        TemporaryFile temporary("tmp_%%%%-%%%%-%%%%-%%%%.def");

        timer.start();
        defs.write_to_checkpt_file(temporary.path());
        cout << " Save as DEFS checkpoint, time taken            = " << timer << endl;
    }

    {
        // Test time for persisting to CEREAL checkpoint file only
        fs::path fs_path(path);
        //   std::cout << "parent path " << fs_path.parent_path() << "\n";
        //   std::cout << "root path " << fs_path.root_path()  << "\n";
        //   std::cout << "root name " << fs_path.root_name()  << "\n";
        //   std::cout << "root directory " << fs_path.root_directory()  << "\n";
        //   std::cout << "relative_path " << fs_path.relative_path()  << "\n";
        //   std::cout << "filename " << fs_path.filename()  << "\n";
        //   std::cout << "stem " << fs_path.stem()  << "\n";
        //   std::cout << "extension " << fs_path.extension()  << "\n";

        std::stringstream ss;
#ifdef DEBUG
        ss << "/var/tmp/ma0/JSON/debug_" << fs_path.stem() << ".json";
#else
        ss << "/var/tmp/ma0/JSON/" << fs_path.stem() << ".json";
#endif

        std::string json_filepath = ss.str();
        Str::replaceall(json_filepath, "\"", ""); // fs_path.stem() seems to add ", so remove them
        // cout << "  json_filepath: " << json_filepath << endl;

        std::remove(json_filepath.c_str());
        timer.start();
        defs.cereal_save_as_checkpt(json_filepath);
        cout << " Save as CEREAL checkpoint, time taken          = " << timer << endl;
    }

    {
        // may need to comment out output for large differences. Will double the time.
        bool do_compare = false;
        timer.start();
        PersistHelper helper;
        bool result = helper.test_defs_checkpt_and_reload(defs, do_compare);
        cout << " Checkpt(DEFS) and reload, time taken           = " << timer << " file_size(" << helper.file_size()
             << ")  result(" << result << ") msg(" << helper.errorMsg() << ")" << endl;
    }

    {
        bool do_compare = false;
        timer.start();
        PersistHelper helper;
        bool result = helper.test_cereal_checkpt_and_reload(defs, do_compare);
        cout << " Checkpt(CEREAL) and reload , time taken        = ";
        cout << timer << " file_size(" << helper.file_size() << ")  result(" << result << ") msg(" << helper.errorMsg()
             << ")" << endl;
    }

    {
        timer.start();
        for (suite_ptr s : defs.suiteVec()) {
            test_find_task_using_path(s.get(), defs);
        }
        cout << " Test all paths can be found. time taken        = " << timer << endl;
    }
    {
        // Time how long it takes for job submission. Must call begin on all suites first.
        timer.start();
        defs.beginAll();
        int count = 10;
        JobsParam jobsParam; // default is not to create jobs, hence only used in testing
        Jobs jobs(&defs);
        for (int i = 0; i < count; i++) {
            jobs.generate(jobsParam);
        }
        cout << " time for 10 jobSubmissions                     = " << timer
             << " jobs:" << jobsParam.submitted().size() << endl;
    }
    {
        // Time how long it takes for post process
        timer.start();
        string errorMsg, warningMsg;
        bool result = defs.check(errorMsg, warningMsg);
        cout << " Time for Defs::check(inlimit resolution)       = " << timer << " result(" << result << ")" << endl;
    }
    {
        // Time how long it takes to delete all nodes/ references. Delete all tasks and then suites/families.
        timer.start();
        std::vector<Task*> tasks;
        defs.getAllTasks(tasks);
        for (Task* ta : tasks) {
            if (!defs.deleteChild(ta)) {
                cout << "Failed to delete task\n";
            }
        }
        tasks.clear();
        defs.getAllTasks(tasks);
        if (!tasks.empty()) {
            cout << "Expected all tasks to be deleted but found " << tasks.size() << "\n";
        }

        std::vector<suite_ptr> vec = defs.suiteVec(); // make a copy, to avoid invalidating iterators
        for (suite_ptr s : vec) {
            std::vector<node_ptr> familyVec = s->nodeVec(); // make a copy, to avoid invalidating iterators
            for (node_ptr f : familyVec) {
                if (!defs.deleteChild(f.get())) {
                    cout << "Failed to delete family\n";
                }
            }
            if (!s->nodeVec().empty()) {
                cout << "Expected all Families to be deleted but found " << s->nodeVec().size() << "\n";
            }
            if (!defs.deleteChild(s.get())) {
                cout << "Failed to delete suite\n";
            }
        }
        if (!defs.suiteVec().empty()) {
            cout << "Expected all Suites to be deleted but found " << defs.suiteVec().size() << "\n";
        }

        cout << " time for deleting all nodes                    = " << timer << endl;
    }
}
