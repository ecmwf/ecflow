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

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/PasswdFile.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

// #define DEBUG_ME 1

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_PasswdFile)

void test_passwd_files(const std::string& directory, bool pass) {
    auto full_path = fs::absolute(directory);

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));

#if DEBUG_ME
    ECF_TEST_DBG(<< "...In directory: " << full_path.relative_path());
#endif

    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; ++dir_itr) {
        try {
            fs::path relPath(directory + "/" + dir_itr->path().filename().string());

            // recurse down directories
            if (fs::is_directory(dir_itr->status())) {
                test_passwd_files(relPath.string(), pass);
                continue;
            }
#if DEBUG_ME
            ECF_TEST_DBG(<< "......Parsing file " << relPath.string());
#endif
            PasswdFile theFile;
            std::string errorMsg;
            bool parsedOk = theFile.load(relPath.string(), false /*debug*/, errorMsg);
            if (pass) {
                // Test expected to pass
                BOOST_CHECK_MESSAGE(parsedOk,
                                    "Failed to parse file " << relPath << "\n"
                                                            << errorMsg << "\n"
                                                            << theFile.dump());
            }
            else {
                // test expected to fail
                BOOST_CHECK_MESSAGE(!parsedOk,
                                    "Parse expected to fail for " << relPath << "\n"
                                                                  << errorMsg << "\n"
                                                                  << theFile.dump());
#if DEBUG_ME
                ECF_TEST_DBG(<< errorMsg);
#endif
            }
        }
        catch (const std::exception& ex) {
            ECF_TEST_DBG(<< dir_itr->path().filename() << " " << ex.what());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_passwd_files) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodPasswdFiles", "libs/core");

    // All the files in this directory are expected to pass
    test_passwd_files(path, true);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_bad_passwd_files) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/badPasswdFiles", "libs/core");

    // All the files in this directory are expected to fail
    test_passwd_files(path, false);
}

BOOST_AUTO_TEST_CASE(test_passwd_empty_file) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodPasswdFiles/empty.passwd", "libs/core");

    PasswdFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    BOOST_REQUIRE_MESSAGE(theFile.passwds().empty(), "expected empty file ");
    BOOST_REQUIRE_MESSAGE(theFile.get_passwd("fred", "host", "port") == std::string(), "expected empty string");
    BOOST_REQUIRE_MESSAGE(theFile.authenticate("fred", ""),
                          "expected to authenticate. TEST CASE with empty password file");
    BOOST_REQUIRE_MESSAGE(!theFile.authenticate("fred", "passwd"), "expected not to authenticate");
    BOOST_REQUIRE_MESSAGE(!theFile.authenticate("", "passwd"), "expected not to authenticate");
    BOOST_REQUIRE_MESSAGE(!theFile.authenticate("", ""), "expected not to authenticate");
}

BOOST_AUTO_TEST_CASE(test_passwd) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodPasswdFiles/ecf.passwd", "libs/core");

    PasswdFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host", "3141"), "expected to pass");
    BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host3", "3143"), "expected to pass");
    BOOST_CHECK_MESSAGE(theFile.check_at_least_one_user_with_host_and_port("host4", "3145"), "expected to pass");
    BOOST_CHECK_MESSAGE(!theFile.check_at_least_one_user_with_host_and_port("xxxx", "3141"), "expected fail ");
    BOOST_CHECK_MESSAGE(!theFile.check_at_least_one_user_with_host_and_port("host", "13141"), "expected fail ");

    // make sure we find all the users and the access right are correct
    //   4.4.0  # comment
    //
    //   # comment
    //   fred host 3141  x12ggg # comment
    //   fred host3 3143 passwd
    //   fred host4 3145 x12ggg
    //   # comment
    //
    //   jake host 3141  x12ggg
    //   tom host3 3143    x12ggg # sdsdsd

    std::vector<Pass_wd> expected_passwds;
    expected_passwds.emplace_back("fred", "host", "3141", PasswordEncryption::encrypt("x12ggg", "fred"));
    expected_passwds.emplace_back("fred", "host3", "3143", PasswordEncryption::encrypt("passwd", "fred"));
    expected_passwds.emplace_back("fred", "host4", "3145", PasswordEncryption::encrypt("x12ggg", "fred"));
    expected_passwds.emplace_back("jake", "host", "3141", PasswordEncryption::encrypt("x12ggg", "jake"));
    expected_passwds.emplace_back("tom", "host3", "3143", PasswordEncryption::encrypt("x12ggg", "tom"));

    BOOST_REQUIRE_MESSAGE(expected_passwds == theFile.passwds(), "expected passwords to match");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
