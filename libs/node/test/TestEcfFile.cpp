/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

boost::test_tools::assertion_result is_testing_on_cray([[maybe_unused]] boost::unit_test::test_unit_id id) {
    return ecf::environment::has("ECFLOW_CRAY_BATCH");
}

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_EcfFile)

BOOST_AUTO_TEST_CASE(test_ecf_file_with_bad_ECF_MICRO) {
    ECF_NAME_THIS_TEST();

    // Create the defs file corresponding to the text below
    // suite suite
    //  task t1
    //     ECF_MICRO ""   # cannot be empty if overridden
    //  task t2
    //     ECF_MICRO "ss" # must be a single char
    // endsuite

    task_ptr task_t1;
    task_ptr task_t2;
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("suite");
        task_t1         = suite->add_task("t1");
        task_t1->add_variable("ECF_MICRO", "");
        task_t2 = suite->add_task("t2");
        task_t2->add_variable("ECF_MICRO", "ss");
    }

    // Check we throw for bad ECF_MICRO chars
    std::string ecf_file_location;
    BOOST_REQUIRE_THROW(EcfFile ecfFile(task_t1.get(), ecf_file_location), std::runtime_error);
    BOOST_REQUIRE_THROW(EcfFile ecfFile(task_t2.get(), ecf_file_location), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_ecf_simple_include_file) {
    ECF_NAME_THIS_TEST();

    // The specific files are specified in ECF_INCLUDE and common files
    // are specified in ECF_HOME. This test will ensure that if the file common.h
    // is not found in ECF_INCLUDE we then look at ECF_HOME

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_simple_include_file"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header   = "%include <simple_head.h>\n";
    string body     = "#body\n";
    string tail     = "%include <simple_tail.h>\n";
    string ecf_file = header;
    ecf_file += body;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of '.usr' and job files
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }

    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file/
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";
    BOOST_CHECK_MESSAGE(job_file_contents == expected_job_file_contents,
                        "Expected\n"
                            << expected_job_file_contents << "' but found \n"
                            << job_file_contents << "'");

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_495) {
    ECF_NAME_THIS_TEST();

    // This tests for a regression where, *NOT* all the include file were processed.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite
    Defs theDefs;
    suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_ECFLOW_495"));
    suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
    task_ptr task_t1 = suite->add_task("t1");

    // PrintStyle style(PrintStyle::STATE);
    // std::cout << theDefs << "\n";

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files
    theDefs.beginAll();

    // Create the ecf file; *NOTE* <a.h> includes <b.h>
    string ecf_file = "%include <a.h>\n";
    ecf_file += "%PAR_EXE:parallel% --gnu --verbose -j $PAR_THR ${run:-0} ::: $(seq %PAR_BEG:0% %PAR_END:50%)\n";

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }

    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file and check job file contents match what we expected
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    std::string expected_job_file_contents =
        "#a.h\n#b.h\nparallel --gnu --verbose -j $PAR_THR ${run:-0} ::: $(seq 0 50)";
    BOOST_CHECK_MESSAGE(job_file_contents == expected_job_file_contents,
                        "Expected\n'" << expected_job_file_contents << "' but found \n'" << job_file_contents << "'");

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ECF_SCRIPT_CMD_ECFLOW_427) {
    ECF_NAME_THIS_TEST();

    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ECF_SCRIPT_CMD_ECFLOW_427"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addVariable(Variable("body", "body"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header = "%include <simple_head.h>\n";
    string body   = "#%body%\n";
    body += "%manual\nThis is a manual\n%end\n";
    body += "%comment\nThis is a comment\n%end\n";
    string tail     = "%include <simple_tail.h>\n";
    string ecf_file = header;
    ecf_file += body;
    ecf_file += tail;

    std::string cmd = "cat %ECF_HOME%/%ECF_NAME%.ecf";
    task_t1->add_variable("ECF_SCRIPT_CMD", cmd);

    /// Now finally the test, use this as this will perform variable expansion on ECF_SCRIPT_CMD
    EcfFile ecfFile = task_t1->locatedEcfFile();

    // *******************************************************************************************************
    // CREATE the file after locatedEcfFile() otherwise it will just use script as is, and NOT ECF_SCRIPT_CMD
    // *******************************************************************************************************
    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    /// Check generation of job files
    JobsParam jobsParam(true); // spawn_jobs = false
    {
        task_t1->update_generated_variables();

        try {
            ecfFile.create_job(jobsParam);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
        }

        string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
        BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

        // Open the job file and check contents
        std::string job_file_contents;
        BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                            "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");
        std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";
        BOOST_CHECK_MESSAGE(job_file_contents == expected_job_file_contents,
                            "Expected\n'" << expected_job_file_contents << "' but found:\n'" << job_file_contents
                                          << "'");
    }

    /// Check pre-processing arbitrary file, should return same result as task pre-processing for job generation
    {
        task_t1->update_generated_variables();

        string ecf_file_location = ecf_home + task_t1->absNodePath() + ".ecf";
        std::vector<std::string> user_edit_file;
        BOOST_CHECK_MESSAGE(File::splitFileIntoLines(ecf_file_location, user_edit_file),
                            "Could not open file " << ecf_file_location);

        string processed_file;
        try {
            ecfFile.pre_process_user_file(user_edit_file, processed_file);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected pre-processing succeed " << e.what());
        }

        // because we have read file, as a vector of lines, expect extra \n at the end
        std::string expected_job_file_contents = "#head.h\n#body\n#tail.h\n";
        BOOST_CHECK_MESSAGE(processed_file == expected_job_file_contents,
                            "Expected\n'" << expected_job_file_contents << "' but found:\n'" << processed_file << "'");
    }

    /// Check manual extraction
    {
        std::string manual;
        try {
            ecfFile.manual(manual);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected manual extraction to succeed " << e.what());
        }

        std::string expected_manual = "This is a manual\n";
        BOOST_CHECK_MESSAGE(manual == expected_manual,
                            "Expected:\n'" << expected_manual << "' but found:\n'" << manual << "'");
    }

    /// Check script
    {
        std::string script;
        try {
            ecfFile.script(script);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected script extraction to succeed " << e.what());
        }
        string the_ecf_file = ecfFile.ecf_file_origin_dump() + "\n" + ecf_file;
        BOOST_CHECK_MESSAGE(script == the_ecf_file,
                            "Expected:\n'" << the_ecf_file << "' but found:\n'" << script << "'");
    }

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_include_file) {
    ECF_NAME_THIS_TEST();

    // The specific files are specified in ECF_INCLUDE and common files
    // are specified in ECF_HOME. This test will ensure that if the file common.h
    // is not found in ECF_INCLUDE we then look at ECF_HOME

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    // edit SLEEPTIME 10
    // edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_include_file"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addVariable(Variable("SLEEPTIME", "1"));
        suite->addVariable(Variable("ECF_CLIENT_EXE_PATH", "a/made/up/path"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header   = "%include <head.h>\n\n";
    string body     = "%include <common.h>\n\n";
    string tail     = "%include <tail.h>\n# ===================================";
    string ecf_file = header;
    ecf_file += body;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of '.usr' and job files
    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file/
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_include_multi_paths_ECFLOW_261) {
    ECF_NAME_THIS_TEST();

    // The specific files are specified in ECF_INCLUDE with multiple paths

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    // edit ECF_INCLUDE "$ECF_HOME/empty_include1:$ECF_HOME/empty_include2:$ECF_HOME/includes:$ECF_HOME/includes2"
    //    task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_include_multi_paths_ECFLOW_261"));
    Defs theDefs;
    {
        suite->addVariable(
            Variable(ecf::environment::ECF_INCLUDE,
                     "$ECF_HOME/empty_include1:$ECF_HOME/empty_include2:$ECF_HOME/includes:$ECF_HOME/includes2"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header   = "%include <head.h>\n\n";
    string body     = "%include <fred.h>\n\n"; // this is only defined in libs/node/test/data/includes2
    string tail     = "%include <tail.h>\n# ===================================";
    string ecf_file = header;
    ecf_file += body;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of job files
    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_include_ECFLOW_274) {
    ECF_NAME_THIS_TEST();

    // Test .ecf scripts with includes like %include "../bill.h"
    // In this case we expect to find bill.h in the same directory as the script

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    // edit ECF_INCLUDE "$ECF_HOME/includes"
    //    task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_include_ECFLOW_274"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header   = "%include <head.h>\n\n";
    string body     = "%include \"./t1.h\"\n\n";
    string tail     = "%include <tail.h>\n# ===================================";
    string ecf_file = header;
    ecf_file += body;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    //   cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // generate bill.h
    string header_file          = "# in t1.h";
    string header_file_location = ecf_home + task_t1->absNodePath() + ".h";
    //   cout << "file_location = " << header_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::create(header_file_location, header_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(header_file_location), "Expected File " << header_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of job files
    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    /// Remove the directories that were generated. This occasionally fails on ecgb and lxg ???
    try {
        fs::remove_all(ecf_home + suite->absNodePath());
    }
    catch (const fs::filesystem_error& e) {
        cout << "Could not remove directory " << ecf_home + suite->absNodePath() << " : " << e.what() << "\n";
    }
}

BOOST_AUTO_TEST_CASE(test_ecf_simple_used_variables) {
    ECF_NAME_THIS_TEST();

    // Test that used variables are as expected
    // This should PRUNE the generated variables from the used variables list
    // Additionally it should NOT affect variables like ESUITE but should ignore generated variable SUITE
    // See File: libs/node/test/data/includes/used_variables.h

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1;
    suite_ptr suite;
    Defs theDefs;
    {
        suite = theDefs.add_suite(Pid::unique_name("test_ecf_simple_used_variables"));
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->add_variable("ESUITE", "suite");
        task_t1 = suite->add_family("f1")->add_task("t1");
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string ecf_file          = "%include <used_variables.h>\n";
    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    string file_with_used_variables;
    ecfFile.edit_used_variables(file_with_used_variables);
    string expected_used_variables =
        "%comment - ecf user variables\nESUITE = suite\n%end - ecf user variables\n%include <used_variables.h>\n";
    BOOST_CHECK_MESSAGE(file_with_used_variables == expected_used_variables,
                        "Expected\n"
                            << expected_used_variables << "\nBut found:\n"
                            << file_with_used_variables);

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_simple_used_variables_with_comments) {
    ECF_NAME_THIS_TEST();

    // Test that used variables are as expected
    // This should PRUNE the generated variables from the used variables list
    // Additionally it should NOT affect variables like ETASK but should ignore generated variable TASK
    // See File: libs/node/test/data/includes/used_variables_with_comments.h
    //
    // This WILL test that when we have user comment and manuals, we can still extract user variables
    // Those variable defined within comments and manuals that are not defined should be ignored

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1;
    suite_ptr suite;
    Defs theDefs;
    {
        suite = theDefs.add_suite(Pid::unique_name("test_ecf_simple_used_variables_with_comments"));
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->add_variable("ETASK", "suite");
        suite->add_variable("FRED", "fred");
        task_t1 = suite->add_family("f1")->add_task("t1");
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string ecf_file          = "%include <used_variables_with_comments.h>\n";
    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    string file_with_used_variables;
    ecfFile.edit_used_variables(file_with_used_variables);
    string expected_used_variables = "%comment - ecf user variables\nETASK = suite\nFRED = fred\n%end - ecf user "
                                     "variables\n%include <used_variables_with_comments.h>\n";
    BOOST_CHECK_MESSAGE(file_with_used_variables == expected_used_variables,
                        "Expected\n"
                            << expected_used_variables << "\nBut found:\n"
                            << file_with_used_variables);

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_simple_used_variables_errors) {
    ECF_NAME_THIS_TEST();

    // Test that used variables are as expected
    // This is similar to test_ecf_simple_used_variables_with_comments
    // BUT we DO NOT define variable FRED, hence we expect failure

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1;
    suite_ptr suite;
    Defs theDefs;
    {
        suite = theDefs.add_suite(Pid::unique_name("test_ecf_simple_used_variables_errors"));
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->add_variable("ETASK", "suite");
        task_t1 = suite->add_family("f1")->add_task("t1");
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string ecf_file          = "%include <used_variables_with_comments.h>\n";
    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    // Expect a throw since %FRED% is not defined, on the suite, but exists in used_variables_with_comments.h
    string file_with_used_variables;
    BOOST_REQUIRE_THROW(ecfFile.edit_used_variables(file_with_used_variables), std::runtime_error);

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_file) {
    ECF_NAME_THIS_TEST();

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create the defs file corresponding to the text below
    // # Test the sms file can be found via ECF_SCRIPT
    // # Note: we have to use relative paths, since these tests are relocatable
    // #
    // suite suite
    //	edit SLEEPTIME 10
    //	edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    NameValueMap expected_used_variables;
    expected_used_variables.insert(std::make_pair(string("VAR1"), string("_val1_")));
    expected_used_variables.insert(std::make_pair(string("VAR2"), string("_val2_")));
    expected_used_variables.insert(std::make_pair(string("VAR2_fred"), string("<ignored>")));

    // Create a defs file, where the task name mirrors the sms files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_file"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addVariable(Variable("SLEEPTIME", "1"));
        suite->addVariable(Variable("ECF_CLIENT_EXE_PATH", "a/made/up/path"));
        for (const NameValueMap::value_type& p : expected_used_variables) {
            task_t1->addVariable(Variable(p.first, p.second));
        }
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME.   ECF_HOME is need to locate to the .ecf files
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string header      = "%include <head.h>\n\n";
    string manual_head = "%manual\n";
    string manual_body = " manual. The contents of the manual\n";
    manual_body += " end.\n";
    string manual_tail  = "%end\n\n";
    string comment_head = "%comment\n";
    string comment_body = " comment. The contents of the comment\n";
    comment_body += " end.\n";
    string comment_tail = "%end\n\n";
    string ecf_body;
    {
        for (std::pair<std::string, std::string> p : expected_used_variables) {
            ecf_body += Ecf::MICRO() + p.first + Ecf::MICRO() + "\n";
        }
        ecf_body += "%VAR3:substitute_var%\n";
    }
    string tail = "\n%include <tail.h>\n# ===================================";

    string ecf_file = header;
    ecf_file += manual_head;
    ecf_file += manual_body;
    ecf_file += manual_tail;
    ecf_file += comment_head;
    ecf_file += comment_body;
    ecf_file += comment_tail;
    ecf_file += ecf_body;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    //	cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Test manual extraction
    /// The manual is manual of all the pre-processed includes
    /// Test: SUP-762 Lines starting with "manually" are not shown in manual
    std::string expected_manual = "#This is the manual from the head.h file\n manual. The contents of the manual\n "
                                  "end.\n#This is the manual from the tail.h file\n";

    string theExtractedManual;
    try {
        ecfFile.manual(theExtractedManual);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, e.what());
    }
    BOOST_CHECK_MESSAGE(theExtractedManual == expected_manual,
                        "Expected \n'" << expected_manual << "' but found \n'" << theExtractedManual << "'");

    /// Test script extraction
    string theExtractedScript;
    try {
        ecfFile.script(theExtractedScript);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, e.what());
    }
    BOOST_CHECK_MESSAGE(theExtractedScript == ecf_file,
                        "\nExpected\n"
                            << ecf_file << "\nsize = " << ecf_file.size() << " but found-----------------\n"
                            << theExtractedScript << "\nsize = " << theExtractedScript.size());

    /// Test User edit script, this should return all the used variables, between %comment -%end
    string file_with_used_variables;
    ecfFile.edit_used_variables(file_with_used_variables);
    //    std::cout << "file_with_used_variables:----------------------------------------------------------------\n" <<
    //    file_with_used_variables << "\n";
    BOOST_CHECK_MESSAGE(file_with_used_variables.find("%comment") == 0,
                        "Expected to find variable %comment on the very first line: but found at: "
                            << file_with_used_variables.find("%comment"));
    for (std::pair<std::string, std::string> p : expected_used_variables) {
        BOOST_CHECK_MESSAGE(file_with_used_variables.find(p.first) != string::npos,
                            "Expected to find variable" << p.first);
    }

    /// Test extraction of all the used variables
    std::vector<string> script_lines;
    Str::split(file_with_used_variables, script_lines, "\n"); //  will ignore empty lines, but will do for this case
    NameValueMap extracted_used_variables;
    EcfFile::extract_used_variables(extracted_used_variables, script_lines);
    for (std::pair<std::string, std::string> p : expected_used_variables) {
        BOOST_CHECK_MESSAGE(extracted_used_variables.find(p.first) != extracted_used_variables.end(),
                            " expected to find variable " << p.first << " in the extracted variables\n");
    }
    //    cout << "Expected:----\n"; for(p:expected_used_variables) { cout << p.first << " " << p.second << "\n";}
    //    cout << "Actual:------\n"; for(p:extracted_used_variables) { cout << p.first << " " << p.second << "\n";}

    /// Test pre-processing
    string pre_processed_file;
    ecfFile.pre_process(pre_processed_file);
    //	cout << "pre_processed_file\n" << pre_processed_file << "\n";
    BOOST_CHECK_MESSAGE(!pre_processed_file.empty(), "Expected file not to be empty");
    BOOST_CHECK_MESSAGE(pre_processed_file.find("%include") == string::npos, "Expected all includes to be removed");

    /// Check generation of '.usr' and job files
    string man_file_location = ecf_home + task_t1->absNodePath() + File::MAN_EXTN();
    string usr_file_location = ecf_home + task_t1->absNodePath() + File::USR_EXTN();
    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    JobsParam jobsParam(true); // spawn_jobs = false
    jobsParam.set_user_edit_variables(extracted_used_variables);
    jobsParam.set_user_edit_file(script_lines);
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }
    BOOST_CHECK_MESSAGE(fs::exists(usr_file_location), "Expected File " << usr_file_location << " to exist");
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    // Open the job file/
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    // Test the contents of the job file.
    //    cout << "\n" << job_file_contents << "\n";
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_PORT%") == string::npos, "Expected variables to be substituted:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_HOST%") == string::npos, "Expected variables to be substituted:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_NAME%") == string::npos, "Expected variables to be substituted:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_PASS%") == string::npos, "Expected variables to be substituted");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_TRYNO%") == string::npos, "Expected variables to be substituted");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%include") == string::npos, "Expected all includes to be expanded");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%manual") == string::npos, "%manual should have been removed");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%comment") == string::npos, "%comment should have been removed:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%end") == string::npos, "%end should have been removed:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ecfmicro") == string::npos, "%ecfmicro should have been removed:");

    /// Remove all the generated files
    fs::remove(man_file_location);
    fs::remove(ecf_file_location);
    fs::remove(usr_file_location);
    fs::remove(job_file_location);
    fs::remove(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_file_includenoop) {
    ECF_NAME_THIS_TEST();

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // This test is used to check that %includenopp are expanded only.
    // There should be NO variable substitution, or removal of comments/manual

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name("test_ecf_file_includenoop"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addVariable(Variable("SLEEPTIME", "1"));
        suite->addVariable(Variable("ECF_CLIENT_EXE_PATH", "a/made/up/path"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }
    // cout << theDefs << "\n";

    // Override ECF_HOME. ECF_HOME is need to locate to the .ecf files
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files
    theDefs.beginAll();

    // generate the ecf file;
    string header   = "%includenopp <head.h>\n";
    string tail     = "%includenopp <tail.h>";
    string ecf_file = header;
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location),
                        "Could not create missing dir " << ecf_file_location << "\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables. Then EcfFile
    task_t1->update_generated_variables();
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of job files
    string man_file_location = ecf_home + task_t1->absNodePath() + File::MAN_EXTN();
    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }

    // Open the job file and check the contents
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected job File " << job_file_location << " to exist");
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");

    // Test the contents of the job file. We expect includenopp to be expanded
    // The contents should be left as is: i.e no pre_processing,hence expect to find %manual %comment, %VARIABLES%
    // cout << "\n" << job_file_contents << "\n";
    BOOST_CHECK_MESSAGE(job_file_contents.find("%includenopp") == string::npos, "Expected all includes to be removed");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_PORT%") != string::npos, "Expected variables as is:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_HOST%") != string::npos, "Expected variables as is:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_NAME%") != string::npos, "Expected variables as is:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_PASS%") != string::npos, "Expected variables as is:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%ECF_TRYNO%") != string::npos, "Expected variables as is:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%manual") != string::npos,
                        "%manual should exist inside %nopp/%end pair:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%comment") != string::npos,
                        "%comment should exist inside %nopp/%end pair:");
    BOOST_CHECK_MESSAGE(job_file_contents.find("%end") != string::npos,
                        "%end associated with comment and manual should exist:");

    // Remove all the generated files
    fs::remove(ecf_file_location);
    fs::remove(man_file_location);
    fs::remove(job_file_location);
    fs::remove(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_ecf_file_override_ECF_JOB) {
    ECF_NAME_THIS_TEST();

    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // This test is used to check that when user has added a variable ECF_JOB
    // to specify the location of the job file, we use that, in preference
    // to generated ECF_JOB for the location of the job file.
    // Note: The directories to the job file should be created by EcfFile

    // SET ECF_HOME
    std::string ecf_home          = File::test_data("libs/node/test/data", "libs/node");
    std::string job_file_location = ecf_home + "/a/made/up/path/t1.job";

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    task_t1->addVariable(Variable("ECF_JOB", job_file_location));
    suite_ptr suite = Suite::create(Pid::unique_name("test_ecf_file_override_ECF_JOB"));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->addVariable(Variable("SLEEPTIME", "1"));
        suite->addVariable(Variable("ECF_CLIENT_EXE_PATH", "a/made/up/path"));
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }
    // cout << theDefs << "\n";

    // Override ECF_HOME. ECF_HOME is need to locate to the .ecf files
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files
    theDefs.beginAll();

    // generate the dummy ecf file;
    string header   = "%include <head.h>\n";
    string tail     = "%include <tail.h>";
    string ecf_file = header;
    ecf_file += "# ";
    ecf_file += tail;

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location),
                        "Could not create missing dir " << ecf_file_location << "\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables. Then EcfFile
    task_t1->update_generated_variables();
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of job files
    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }

    // Open the job file and check the contents
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected job File " << job_file_location << " to exist");
    std::string job_file_contents;
    BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                        "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");
    BOOST_CHECK_MESSAGE(!job_file_contents.empty(), "Job should not be empty");

    // Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
    fs::remove_all(ecf_home + "/a");

    /// Destroy System singleton to avoid valgrind from complaining
    System::destroy();
}

BOOST_AUTO_TEST_CASE(test_manual_files) {
    ECF_NAME_THIS_TEST();

    // The specific files are specified in ECF_INCLUDE and common files are specified in ECF_HOME.

    // SET ECF_HOME
    std::string ecf_home = File::test_data("libs/node/test/data/SMSHOME", "libs/node");

    // Create the defs file corresponding to the text below
    // suite suite
    // edit SLEEPTIME 10
    // edit ECF_INCLUDE $ECF_HOME/includes
    //  family
    //     task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    Defs theDefs;
    suite_ptr suite = theDefs.add_suite("suite"); // ** relies on name of suite, in SMSHOME/suite
    suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/../includes"));
    family_ptr family = suite->add_family("family");
    task_ptr task_t1  = family->add_task("t1");

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // Create the generated variables
    task_t1->update_generated_variables();

    // PrintStyle style(PrintStyle::STATE);
    // std::cout << theDefs << "\n";

    /// Now finally the test

    // Task
    {
        std::string manual;
        EcfFile ecf_file = task_t1->locatedEcfFile(); // will throw std::runtime_error for errors
        ecf_file.manual(manual);                      // will throw std::runtime_error for errors
        BOOST_REQUIRE_MESSAGE(!manual.empty(), "Manual not found");
        BOOST_CHECK_MESSAGE(manual.find("ECF_MICRO=%") != std::string::npos,
                            "Variable pre-processing failed during manual extraction");
        BOOST_CHECK_MESSAGE(manual.find("manual-1") != std::string::npos,
                            "Pre-processing of ecfmicro in manuals failed, expected to find string 'manual-1'\n"
                                << manual);
        BOOST_CHECK_MESSAGE(manual.find("end-1") != std::string::npos,
                            "Pre-processing of ecfmicro in manuals failed, expected to find string 'end-1'\n"
                                << manual);
        BOOST_CHECK_MESSAGE(manual.find("Test manual files are pre-processed") != std::string::npos,
                            "%include <manual.h> pre-processing failed inside manual->end\n"
                                << manual);
    }

    {
        // Family, Check the suite manuals are pre-processed. i.e %includes are expanded
        // When the node container manual(family or suite) '.man' file, has content but *NO* %manual->%end directives
        // Just pre-process and return file as is. Since the whole file is a manual.
        std::string man_file = ecf_home + family->absNodePath() + File::MAN_EXTN();
        EcfFile ecf_file(family.get(), man_file);

        std::string manual;
        ecf_file.manual(manual);
        // cout << manual << "\n";
        BOOST_CHECK_MESSAGE(!manual.empty(), "Manual not found");
        BOOST_CHECK_MESSAGE(manual.find("Test manual files are pre-processed") != std::string::npos,
                            "Pre-processing in manual failed");
        BOOST_CHECK_MESSAGE(manual.find("Special case where there are no manual directives") != std::string::npos,
                            "family manual extraction failed");
    }

    {
        // Suite, Check the suite manuals are pre-processed. i.e %includes are expanded
        std::string man_file = ecf_home + suite->absNodePath() + File::MAN_EXTN();
        EcfFile ecf_file(suite.get(), man_file);

        std::string manual;
        ecf_file.manual(manual);
        // cout << manual << "\n";
        BOOST_CHECK_MESSAGE(!manual.empty(), "Manual not found");
        BOOST_CHECK_MESSAGE(manual.find("Test manual files are pre-processed") != std::string::npos,
                            "Pre-processing in manual failed");
        BOOST_CHECK_MESSAGE(manual.find("suite manual") != std::string::npos, "Suite manual extraction failed");
    }
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_672) {
    ECF_NAME_THIS_TEST();

    // test for recursive includes that are not recursive

    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite ECFLOW_672
    //  edit ECF_INCLUDE $ECF_HOME/ECFLOW_672
    //  task t
    // endsuite
    Defs theDefs;
    suite_ptr suite = theDefs.add_suite("ECFLOW_672");
    suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/ECFLOW_672"));
    task_ptr task_t1 = suite->add_task("t");

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the ecf files
    theDefs.beginAll();

    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    JobsParam jobsParam(true); // spawn_jobs = false
    try {
        ecfFile.create_job(jobsParam);
    }
    catch (std::exception& e) {
        BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
    }

    string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
    BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

    /// Remove generate file
    fs::remove(job_file_location);
}

static void basic_test_template(const std::string& test_name,
                                const std::string& ecf_file1,
                                const std::string& expected_job_file_contents,
                                const std::string& ecf_micro = "",
                                bool expect_success          = true) {
    // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
    if (ecf::environment::has("ECFLOW_CRAY_BATCH")) {
        cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
        return;
    }

    // Create the defs file corresponding to the text below
    // suite suite
    //  edit ECF_INCLUDE $ECF_HOME/includes
    //  task t1
    // endsuite

    // Create a defs file, where the task name mirrors the ecf files in the given directory
    task_ptr task_t1 = Task::create("t1");
    suite_ptr suite  = Suite::create(Pid::unique_name(test_name));
    Defs theDefs;
    {
        suite->addVariable(Variable(ecf::environment::ECF_INCLUDE, "$ECF_HOME/includes"));
        suite->add_variable("simple", "simple");
        suite->add_variable("tail", "tail");
        if (!ecf_micro.empty())
            suite->add_variable("ECF_MICRO", ecf_micro);
        suite->addTask(task_t1);
        theDefs.addSuite(suite);
    }

    // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
    // or when file does not exist in ECF_INCLUDE
    std::string ecf_home = File::test_data("libs/node/test/data", "libs/node");
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    /// begin , will cause creation of generated variables. The generated variables
    /// are use in client scripts and used to locate the sms files
    theDefs.beginAll();

    // generate the ecf file;
    string ecf_file          = ecf_file1;
    string ecf_file_location = ecf_home + task_t1->absNodePath() + File::ECF_EXTN();
    // cout << "file_location = " << ecf_file_location << "\n";
    BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location), "Could not create missing dir\n");

    string errormsg;
    BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file, errormsg), errormsg);
    BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");

    // Create the generated variables
    task_t1->update_generated_variables();

    /// Now finally the test
    EcfFile ecfFile(task_t1.get(), ecf_file_location);

    /// Check generation of '.usr' and job files
    JobsParam jobsParam(true); // spawn_jobs = false
    if (expect_success) {
        std::string file_with_used_variables;
        try {
            ecfFile.edit_used_variables(file_with_used_variables);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected edit_used_variables to succeed " << e.what());
        }

        try {
            ecfFile.create_job(jobsParam);
        }
        catch (std::exception& e) {
            BOOST_CHECK_MESSAGE(false, "Expected job creation to succeed " << e.what());
        }

        string job_file_location = ecf_home + task_t1->absNodePath() + File::JOB_EXTN() + task_t1->tryNo();
        BOOST_CHECK_MESSAGE(fs::exists(job_file_location), "Expected File " << job_file_location << " to exist");

        // Open the job file, make sure it matches our expected file
        std::string job_file_contents;
        BOOST_CHECK_MESSAGE(File::open(job_file_location, job_file_contents),
                            "Could not open job file " << job_file_location << " (" << strerror(errno) << ")");
        BOOST_CHECK_MESSAGE(job_file_contents == expected_job_file_contents,
                            "Expected\n"
                                << expected_job_file_contents << "' but found \n"
                                << job_file_contents << "'");
    }
    else {
        BOOST_REQUIRE_THROW(ecfFile.create_job(jobsParam), std::runtime_error);
    }

    /// Remove all the generated files
    fs::remove_all(ecf_home + suite->absNodePath());
}

BOOST_AUTO_TEST_CASE(test_includeonce) {
    ECF_NAME_THIS_TEST();

    // simplest test of include once, no hierarchy
    // generate the ecf file;
    string ecf_file = "%includeonce <simple_head.h>\n";
    ecf_file += "%includeonce <simple_head.h>\n";
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%includeonce <simple_tail.h>\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents = "#head.h\n#head.h\n#body\n#tail.h\n#tail.h";

    basic_test_template("test_includeonce", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_includeonce_hierarchical) {
    ECF_NAME_THIS_TEST();

    // Test the pre-processing is done depth first, ( < 4.1.1 it was done breadth first)
    // generate the ecf file;
    string ecf_file = "%includeonce <AA.h>\n";
    ecf_file += "%includeonce <BB.h>\n";
    ecf_file += "%includeonce <CC.h>\n";

    std::string expected_job_file_contents = "#startAA\n#startBB\n#CC\n#endBB\n#endAA";

    basic_test_template("test_includeonce_hierarchical", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_include_with_variables) {
    ECF_NAME_THIS_TEST();

    // Test includes with variables %include <%head%.h> ECFLOW-765

    string ecf_file;
    ecf_file += "%include <%simple%_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%include <simple_%tail%.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_include_with_variables", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_include_with_variable_alternative) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include %FRED:<simple_head.h>%\n";
    ecf_file += "#body\n";
    ecf_file += "%include %TAIL:<simple_tail.h>%\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_include_with_variable_alternative", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_include_with_variables_change_micro) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%ecfmicro &\n"; // ecf_micro in script OVERRIDES ECF_MICRO variable, but *only* in script
    ecf_file += "&include <&simple&_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "&ecfmicro *\n";
    ecf_file += "*include <simple_*tail*.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_include_with_variables_change_micro", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_script_override_ecf_micro) {
    ECF_NAME_THIS_TEST();

    string ecf_file;             // ecf_micro in script OVERRIDES ECF_MICRO variable, but *only* in script
    ecf_file += "$simple$\n";    // ECF_MICRO is set to $
    ecf_file += "$ecfmicro &\n"; // ecfmicro change from $ -> &
    ecf_file += "&include <&simple&_head.h>\n";
    ecf_file += "#body$fred&simple&\n";
    ecf_file += "&ecfmicro *\n"; // ecfmicro change from & -> *
    ecf_file += "*include <simple_*tail*.h>\n";

    std::string expected_job_file_contents = "simple\n#head.h\n#body$fredsimple\n#tail.h";

    basic_test_template("test_script_override_ecf_micro", ecf_file, expected_job_file_contents, "$");
}

BOOST_AUTO_TEST_CASE(test_mistyped_ecf_micro) {
    ECF_NAME_THIS_TEST();

    // same test as above, but we have mistyped ecf_micro. Make sure we don't ignore this.
    string ecf_file;
    ecf_file += "$simple$\n";
    ecf_file += "$ecf_micro &\n"; // ecfmicro has been mistyped
    ecf_file += "&include <&simple&_head.h>\n";
    ecf_file += "#body$fred&simple&\n";
    ecf_file += "&ecf_micro *\n";
    ecf_file += "*include <simple_*tail*.h>\n";

    std::string expected_job_file_contents = "simple\n#head.h\n#body$fredsimple\n#tail.h";

    basic_test_template("test_mistyped_ecf_micro", ecf_file, expected_job_file_contents, "$", false);
}

BOOST_AUTO_TEST_CASE(test_include_with_variables_mismatched_micros) {
    ECF_NAME_THIS_TEST();

    // generate the ecf file, where include file has missmatched ecf_micro
    string ecf_file;
    ecf_file += "%include <%simple%_head%.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%include <simple_%tail%.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_include_with_variables_mismatched_micros",
                        ecf_file,
                        expected_job_file_contents,
                        "" /*ecf_micro*/,
                        false /*expect failure*/);
}

BOOST_AUTO_TEST_CASE(test_include_with_variable_not_defined) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include %simple_head.h%\n";
    ecf_file += "#body\n";
    ecf_file += "%include <simple_%tail%.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_include_with_variable_not_defined",
                        ecf_file,
                        expected_job_file_contents,
                        "" /*ecf_micro*/,
                        false /*expect failure*/);
}

BOOST_AUTO_TEST_CASE(test_nopp_preserves_contents) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%nopp\n";
    ecf_file += "%comment\n";
    ecf_file += "--comment\n";
    ecf_file += "%end\n";
    ecf_file += "%manual\n";
    ecf_file += "--manual\n";
    ecf_file += "%end\n";
    ecf_file += "%end\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents =
        "#head.h\n#body\n%comment\n--comment\n%end\n%manual\n--manual\n%end\n#tail.h";

    basic_test_template("test_nopp_preserves_contents", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_nopp_preserves_contents2) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%nopp\n";
    ecf_file += "%ecfmicro @\n";
    ecf_file += "%end\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n%ecfmicro @\n#tail.h";

    basic_test_template("test_nopp_preserves_contents2", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_comment_and_manual_removal) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%comment\n";
    ecf_file += "--comment\n";
    ecf_file += "%end\n";
    ecf_file += "%manual\n";
    ecf_file += "--manual\n";
    ecf_file += "%end\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body\n#tail.h";

    basic_test_template("test_comment_and_manual_removal", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_unterminated_manual) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "#body\n";
    ecf_file += "%comment\n";
    ecf_file += "--comment\n";
    ecf_file += "%end\n";
    ecf_file += "%manual\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents = "#head.h\n#body%manual\n#tail.h";

    basic_test_template("test_unterminated_manual", ecf_file, expected_job_file_contents, "", false);
}

BOOST_AUTO_TEST_CASE(test_ecf_micro_with_comments_ECFLOW_1686) {
    ECF_NAME_THIS_TEST();

    string ecf_file;
    ecf_file += "%include <simple_head.h>\n";
    ecf_file += "var=%XXXX:2 # fred%\n";
    ecf_file += "# %comment\n";
    ecf_file += "var=%X:2 # CC% # comment\n";
    ecf_file += "%include <simple_tail.h>\n";

    std::string expected_job_file_contents = "#head.h\nvar=2 # fred\n# %comment\nvar=2 # CC # comment\n#tail.h";

    basic_test_template("test_ecf_micro_with_comments_ECFLOW_1686", ecf_file, expected_job_file_contents);
}

BOOST_AUTO_TEST_CASE(test_ecf_file_resolve_single_ecf_include_with_dollar) {
    ECF_NAME_THIS_TEST();

    Defs d;
    suite_ptr s = d.add_suite("suite");                      // suite suite
    s->add_variable("CORE", "/path/to/core");                //   edit CORE /path/to/core
    s->add_variable("ECF_INCLUDE", "$CORE/%SUITE%/include"); //   edit ECF_INCLUDE $CORE/include
    task_ptr t1 = s->add_task("t1");                         //   task t1

    std::string ecf_file_location;
    EcfFile ecf(t1.get(), ecf_file_location);

    auto ecf_include_paths = EcfFile::get_ecf_include_paths(ecf);
    BOOST_REQUIRE_EQUAL(ecf_include_paths.size(), 1u);
    BOOST_REQUIRE_EQUAL(ecf_include_paths[0], "/path/to/core/suite/include");
}

BOOST_AUTO_TEST_CASE(test_ecf_file_resolve_single_ecf_include) {
    ECF_NAME_THIS_TEST();

    Defs d;
    suite_ptr s = d.add_suite("suite");                       // suite suite
    s->add_variable("CORE", "/path/to/core");                 //   edit CORE /path/to/core
    s->add_variable("ECF_INCLUDE", "%CORE%/%SUITE%/include"); //   edit ECF_INCLUDE %CORE%/%SUITE%/include
    task_ptr t1 = s->add_task("t1");                          //   task t1

    std::string ecf_file_location;
    EcfFile ecf(t1.get(), ecf_file_location);

    auto ecf_include_paths = EcfFile::get_ecf_include_paths(ecf);
    BOOST_REQUIRE_EQUAL(ecf_include_paths.size(), 1u);
    BOOST_REQUIRE_EQUAL(ecf_include_paths[0], "/path/to/core/suite/include");
}

BOOST_AUTO_TEST_CASE(test_ecf_file_resolve_multiple_ecf_include) {
    ECF_NAME_THIS_TEST();

    Defs d;
    suite_ptr s = d.add_suite("suite");                                   // suite suite
    s->add_variable("CORE", "/path/to/core");                             //   edit CORE /path/to/core
    s->add_variable("ECF_INCLUDE", "%CORE%/%SUITE%/include:%CORE%/more"); //   edit ECF_INCLUDE %CORE%/%SUITE%...
    task_ptr t1 = s->add_task("t1");                                      //   task t1

    std::string location;
    EcfFile ecf(t1.get(), location);

    auto ecf_include_paths = EcfFile::get_ecf_include_paths(ecf);
    BOOST_REQUIRE_EQUAL(ecf_include_paths.size(), 2u);
    BOOST_REQUIRE_EQUAL(ecf_include_paths[0], "/path/to/core/suite/include");
    BOOST_REQUIRE_EQUAL(ecf_include_paths[1], "/path/to/core/more");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
