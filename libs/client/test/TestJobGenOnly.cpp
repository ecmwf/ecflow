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

#include <boost/test/unit_test.hpp> // IWYU pragma: keep

#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_JobGenOnly)

//  Tests the Job generation against the OLD sms
BOOST_AUTO_TEST_CASE(test_jobgenonly) {
    ECF_NAME_THIS_TEST();

    // Define paths to ECF_HOME and location of the defs file

    std::string defsFile = File::test_data("libs/client/test/data/jobgenonly.def", "libs/client");
    std::string ecf_home = File::test_data("libs/client/test/data/ECF_HOME", "libs/client");

    /// Remove existing job file if any.
    /// 	Job file location may NOT be same as ecf file.
    /// 	The default Job location is defined by generated variable ECF_JOB
    /// 	ECF_JOB = ECF_HOME/<abs node path>.job<try_no>.
    std::vector<std::string> generatedFiles;
    generatedFiles.reserve(5);
    std::string t1_job = ecf_home + "/suite/family/t1.job0";
    generatedFiles.push_back(t1_job);
    std::string t2_job = ecf_home + "/suite/family/t2.job0";
    generatedFiles.push_back(t2_job);
    std::string t3_job = ecf_home + "/suite/family/t3.job0";
    generatedFiles.push_back(t3_job);

    // See EcfFile.cpp: To enable generation of man files, for test. i.e DEBUG_MAN_FILE
#ifdef DEBUG_MAN_FILE
    std::string t1_man = ecf_home + "/suite/family/t1.man";
    generatedFiles.push_back(t1_man);
    std::string t3_man = ecf_home + "/suite/family/t3.man";
    generatedFiles.push_back(t3_man);
#endif
    for (const std::string& s : generatedFiles) {
        fs::remove(s);
    }
    for (const std::string& s : generatedFiles) {
        BOOST_REQUIRE_MESSAGE(!fs::exists(s), "Could not delete file " << s);
    }

    // Load the defs file 'jobgenonly.def'
    Defs theDefs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(theDefs.restore(defsFile, errorMsg, warningMsg), errorMsg);

    // Override ECF_HOME. ECF_HOME is needed to locate to the .ecf files
    theDefs.set_server().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);

    // provide definition of ECF_CLIENT. This should replace smsinit, smscomplete, smsevent,etc
    // with path to the ecf client
    std::string clientPath = File::find_ecf_client_path();
    BOOST_REQUIRE_MESSAGE(!clientPath.empty(), "Could not find path to client executable");
    theDefs.set_server().add_or_update_user_variables("ECF_CLIENT", clientPath);

    // JobCreationCtrl is used control what node we generate the jobs for:
    // Since we have not set the node on it, we force job generation for all tasks
    job_creation_ctrl_ptr jobCtrl = std::make_shared<JobCreationCtrl>();
    theDefs.check_job_creation(jobCtrl);
    BOOST_REQUIRE_MESSAGE(jobCtrl->get_error_msg().empty(), jobCtrl->get_error_msg());
    BOOST_REQUIRE_MESSAGE(jobCtrl->fail_submittables().empty(), "Expected no failing tasks");

    // Check if jobs file were generated.
    for (const std::string& s : generatedFiles) {
        BOOST_REQUIRE_MESSAGE(fs::exists(s), "File " << s << " should have been created");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
