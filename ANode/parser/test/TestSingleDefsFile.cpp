//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <fstream>
#include <iostream>
#include <string>

#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/timer/timer.hpp>

#include "Defs.hpp"
#include "DefsStructureParser.hpp"
#include "Family.hpp"
#include "File.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "Log.hpp"
#include "NodeContainer.hpp"
#include "PersistHelper.hpp"
#include "PrintStyle.hpp"
#include "Str.hpp"
#include "Suite.hpp"
#include "System.hpp"
#include "Task.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE(ParserTestSuite)

// This test is used to find a task given a path of the form:
// 	  suite/family/task
//    suite/family/family/task
void test_find_task_using_path(NodeContainer* f, const Defs& defs) {
    BOOST_CHECK_MESSAGE(f == defs.findAbsNode(f->absNodePath()).get(),
                        "Could not find path " << f->absNodePath() << "\n");

    for (node_ptr t : f->nodeVec()) {
        BOOST_CHECK_MESSAGE(t.get() == defs.findAbsNode(t->absNodePath()).get(),
                            "Could not find path " << t->absNodePath() << "\n");
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

double get_seconds(const boost::timer::nanosecond_type& elapsed) {
    return static_cast<double>(boost::chrono::nanoseconds(elapsed).count()) * boost::chrono::nanoseconds::period::num /
           boost::chrono::nanoseconds::period::den;
}

BOOST_AUTO_TEST_CASE(test_single_defs) {
    boost::timer::auto_cpu_timer auto_cpu_timer;

    std::string path      = File::test_data("ANode/parser/test/data/single_defs/mega.def", "parser");
    size_t mega_file_size = fs::file_size(path);
    cout << "AParser:: ...test_single_defs " << path << " file_size(" << mega_file_size << ")\n";

    // Time parse/resolve dependencies: This will need to be #defined depending on the platform
    // Change for file_iterator to plain string halved the time taken to load operation suite
    boost::timer::cpu_timer timer;

#ifdef DEBUG
    #if defined(HPUX) || defined(_AIX)
    double expectedTimeForParse                   = 15.0;
    double expectedTimeForParseOnly               = 10.0;
    double expectedTimeForResolveDependencies     = 3.5; // this is time for 10 job submissions
    double checkExprAndLimits                     = 1.0;
    double expectedTimeForFindAllPaths            = 7.2;
    double expectedTimeForDefsPersistOnly         = 6;
    double expectedTimeForDefsPersistAndReload    = 24;
    double expectedTimeForCheckPtPersistAndReload = 27;
    #else
    double expectedTimeForParse                   = 4.5;
    double expectedTimeForParseOnly               = 2.0;
    double expectedTimeForResolveDependencies     = 0.5; // this is time for 10 job submissions
    double checkExprAndLimits                     = 0.5;
    double expectedTimeForFindAllPaths            = 1.2;
    double expectedTimeForDefsPersistOnly         = 2;
    double expectedTimeForDefsPersistAndReload    = 4.5;
    double expectedTimeForCheckPtPersistAndReload = 8.0;
    #endif
#else
    #if defined(HPUX) || defined(_AIX)
    double expectedTimeForParse                   = 7.8;
    double expectedTimeForParseOnly               = 5.0;
    double expectedTimeForResolveDependencies     = 3.5; // this is time for 10 job submissions
    double checkExprAndLimits                     = 1.0;
    double expectedTimeForFindAllPaths            = 4.5;
    double expectedTimeForDefsPersistOnly         = 3.5;
    double expectedTimeForDefsPersistAndReload    = 9.5;
    double expectedTimeForCheckPtPersistAndReload = 11.5;
    #else
    double expectedTimeForParse                   = 1.2;
    double expectedTimeForParseOnly               = 0.6;
    double expectedTimeForResolveDependencies     = 0.2; // this is time for 10 job submissions
    double checkExprAndLimits                     = 0.1;
    double expectedTimeForFindAllPaths            = 0.58;
    double expectedTimeForDefsPersistOnly         = 0.4;
    double expectedTimeForDefsPersistAndReload    = 1.4;
    double expectedTimeForCheckPtPersistAndReload = 1.6;
    #endif
#endif
    // ****************************************************************************************
    // Please note: that for Parsing: that the predominate time is taken in creating the AST/ and checking

    /// If this is moved below, we get some caching affect, with the persist and reload timing
    Defs defs;
    {
        timer.start();
        std::string errorMsg, warningMsg;
        BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), errorMsg);
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForParse,
                            "Performance regression, expected < " << expectedTimeForParse
                                                                  << " seconds for parse/node tree creation but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Parsing Node tree and AST creation time                = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForParse << ")" << endl;
    }
    {
        Defs local_defs;
        timer.start();
        TestDefsStructureParser checkPtParser(&local_defs, path);
        std::string errorMsg;
        BOOST_REQUIRE_MESSAGE(checkPtParser.do_parse_file(errorMsg), errorMsg);
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForParse,
                            "Performance regression, expected < " << expectedTimeForParseOnly
                                                                  << " seconds for parse/node tree creation but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Parsing Node tree *only* time                          = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForParseOnly << ")" << endl;
    }
    {
        timer.start();
        for (suite_ptr s : defs.suiteVec()) {
            test_find_task_using_path(s.get(), defs);
        }
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForFindAllPaths,
                            "Performance regression, expected < " << expectedTimeForFindAllPaths
                                                                  << " seconds to find all paths, but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Test all paths can be found. time taken                = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForFindAllPaths << ")" << endl;
    }

    {
        // Test time for persisting to defs file only
#ifdef DEBUG
        std::string tmpFilename = "tmp_d.def";
#else
        std::string tmpFilename = "tmp.def";
#endif

        timer.start();
        PrintStyle style(PrintStyle::DEFS);
        std::ofstream ofs(tmpFilename.c_str());
        ofs << defs;
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForDefsPersistOnly,
                            "Performance regression, expected < " << expectedTimeForDefsPersistOnly
                                                                  << " to persist defs file, but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Persist only, time taken                               = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForDefsPersistOnly << ")" << endl;

        std::remove(tmpFilename.c_str());
    }

    {
        // may need to comment out output for large differences. Will double the time.
        timer.start();
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, PrintStyle::DEFS), helper.errorMsg());
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForDefsPersistAndReload,
                            "Performance regression, expected < " << expectedTimeForDefsPersistAndReload
                                                                  << " seconds to persist and reload, but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Persist and reload(DEFS) and compare, time taken       = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForDefsPersistAndReload << ")"
             << " file_size(" << helper.file_size() << ")" << endl;
    }
    {
        timer.start();
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, PrintStyle::STATE), helper.errorMsg());
        cout << " Persist and reload(STATE) and compare, time taken      = " << timer.format(3, Str::cpu_timer_format())
             << " file_size(" << helper.file_size() << ")" << endl;
    }
    {
        timer.start();
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, PrintStyle::MIGRATE), helper.errorMsg());
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForDefsPersistAndReload,
                            "Performance regression, expected < "
                                << expectedTimeForDefsPersistAndReload
                                << " seconds to persist and reload *state*, but found "
                                << timer.format(3, Str::cpu_timer_format()));
        cout << " Persist and reload(MIGRATE) and compare, time taken    = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForDefsPersistAndReload << ")"
             << " file_size(" << helper.file_size() << ")" << endl;

        // each platform will have a slightly different size, since the server environment variables
        // will be different, i.e host, pid, i.e check point etc, encompasses the host name, which will be different
        BOOST_CHECK_MESSAGE(helper.file_size() <= 6679000,
                            "File size regression expected <= 6679000 but found " << helper.file_size());
    }
    {
        timer.start();
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, PrintStyle::NET), helper.errorMsg());
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForDefsPersistAndReload,
                            "Performance regression, expected < "
                                << expectedTimeForDefsPersistAndReload
                                << " seconds to persist and reload *state*, but found "
                                << timer.format(3, Str::cpu_timer_format()));
        cout << " Persist and reload(NET) and compare, time taken        = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << expectedTimeForDefsPersistAndReload << ")"
             << " file_size(" << helper.file_size() << ")" << endl;

        // each platform will have a slightly different size, since the server environment variables
        // will be different, i.e host, pid, i.e check point etc, encompasses the host name, which will be different
        BOOST_CHECK_MESSAGE(helper.file_size() <= 6679000,
                            "File size regression expected <= 6679000 but found " << helper.file_size());
    }

    {
        timer.start();
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_cereal_checkpt_and_reload(defs, true), helper.errorMsg());
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForCheckPtPersistAndReload,
                            "Performance regression, expected < " << expectedTimeForCheckPtPersistAndReload
                                                                  << " seconds to persist and reload, but found "
                                                                  << timer.format(3, Str::cpu_timer_format()));
        cout << " Checkpt(CEREAL) and reload and compare, time taken     = ";
        cout << timer.format(3, Str::cpu_timer_format()) << " < limit(" << expectedTimeForCheckPtPersistAndReload << ")"
             << " file_size(" << helper.file_size() << ")" << endl;
    }

    {
        // Time how long it takes for job submission. Must call begin on all suites first.
        timer.start();
        defs.beginAll();
        int count = 10;
        JobsParam jobsParam; // default is *not* to create jobs, and *not* to spawn jobs, hence only used in testing
        Jobs jobs(&defs);
        for (int i = 0; i < count; i++) {
            jobs.generate(jobsParam);
        }
        cout << " " << count
             << " jobSubmissions                                      = " << timer.format(3, Str::cpu_timer_format())
             << "s jobs:" << jobsParam.submitted().size() << " < limit(" << expectedTimeForResolveDependencies << ")"
             << endl;
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < expectedTimeForResolveDependencies,
                            "jobSubmission Performance regression, expected < "
                                << expectedTimeForResolveDependencies
                                << " seconds for resolving dependencies but found "
                                << timer.format(3, Str::cpu_timer_format()));
    }

    {
        // Time how long it takes for post process
        timer.start();
        string errorMsg, warningMsg;
        BOOST_CHECK(defs.check(errorMsg, warningMsg));
        cout << " Defs::check (inlimit resolution)                       = " << timer.format(3, Str::cpu_timer_format())
             << " < limit(" << checkExprAndLimits << ")" << endl;
        BOOST_CHECK_MESSAGE(get_seconds(timer.elapsed().user) < checkExprAndLimits,
                            "Defs::check Performance regression, expected < "
                                << checkExprAndLimits << " seconds for resolving dependencies but found "
                                << timer.format(3, Str::cpu_timer_format()));
    }

    {
        // Time how long it takes to delete all nodes/ references. Delete all tasks and then suites/families.
        timer.start();
        std::vector<Task*> tasks;
        defs.getAllTasks(tasks);
        BOOST_CHECK_MESSAGE(tasks.size() > 0, "Expected > 0 tasks but found " << tasks.size());
        for (Task* t : tasks) {
            BOOST_REQUIRE_MESSAGE(defs.deleteChild(t), " Failed to delete task");
        }
        tasks.clear();
        defs.getAllTasks(tasks);
        BOOST_REQUIRE_MESSAGE(tasks.empty(), "Expected all tasks to be deleted but found " << tasks.size());

        std::vector<suite_ptr> vec = defs.suiteVec(); // make a copy, to avoid invalidating iterators
        BOOST_CHECK_MESSAGE(vec.size() > 0, "Expected > 0 Suites but found " << vec.size());
        for (suite_ptr s : vec) {
            std::vector<node_ptr> familyVec = s->nodeVec(); // make a copy, to avoid invalidating iterators
            for (node_ptr f : familyVec) {
                BOOST_REQUIRE_MESSAGE(defs.deleteChild(f.get()), " Failed to delete family");
            }
            BOOST_REQUIRE_MESSAGE(s->nodeVec().empty(),
                                  "Expected all Families to be deleted but found " << s->nodeVec().size());
            BOOST_REQUIRE_MESSAGE(defs.deleteChild(s.get()), " Failed to delete suite");
        }
        BOOST_REQUIRE_MESSAGE(defs.suiteVec().empty(),
                              "Expected all Suites to be deleted but found " << defs.suiteVec().size());

        cout << " time for deleting all nodes                            = " << timer.format(3, Str::cpu_timer_format())
             << endl;
    }

    // Explicitly destroy, To keep valgrind happy
    Log::destroy();
    System::destroy();

    // cout << "Printing Defs \n";
    // PrintStyle style(PrintStyle::DEFS);
    //	std::cout << defs;
}

BOOST_AUTO_TEST_SUITE_END()
