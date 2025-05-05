/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/WhiteListFile.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

// #define DEBUG_ME 1

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_WhiteListFile)

void test_white_list_files(const std::string& directory, bool pass) {
    auto full_path = fs::absolute(directory);

    BOOST_CHECK(fs::exists(full_path));
    BOOST_CHECK(fs::is_directory(full_path));

    // std::cout << "\nIn directory: " << full_path.directory_string() << "\n\n";
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_itr(full_path); dir_itr != end_iter; ++dir_itr) {
        try {
            fs::path relPath(directory + "/" + dir_itr->path().filename().string());

            // recurse down directories
            if (fs::is_directory(dir_itr->status())) {
                test_white_list_files(relPath.string(), pass);
                continue;
            }

#if DEBUG_ME
            std::cout << "......Parsing file " << relPath.string() << "\n";
#endif

            WhiteListFile theFile;
            std::string errorMsg;
            bool parsedOk = theFile.load(relPath.string(), false /*debug*/, errorMsg);
            if (pass) {
                // Test expected to pass
                BOOST_CHECK_MESSAGE(parsedOk,
                                    "Failed to parse file " << relPath << "\n"
                                                            << errorMsg << "\n"
                                                            << theFile.dump_valid_users());
            }
            else {
                // test expected to fail
                BOOST_CHECK_MESSAGE(!parsedOk,
                                    "Parse expected to fail for " << relPath << "\n"
                                                                  << errorMsg << "\n"
                                                                  << theFile.dump_valid_users());
#if DEBUG_ME
                cout << "\n" << errorMsg << "\n";
#endif
            }
        }
        catch (const std::exception& ex) {
            std::cout << dir_itr->path().filename() << " " << ex.what() << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_for_good_white_list_files) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles", "libs/core");

    // All the files in this directory are expected to pass
    test_white_list_files(path, true);
}

BOOST_AUTO_TEST_CASE(test_parsing_for_bad_white_list_files) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/badWhiteListFiles", "libs/core");

    // All the files in this directory are expected to fail
    test_white_list_files(path, false);
}

BOOST_AUTO_TEST_CASE(test_white_list_default) {
    ECF_NAME_THIS_TEST();

    WhiteListFile theFile;

    BOOST_REQUIRE_MESSAGE(0 == theFile.read_access_size(),
                          "expected 0 users with read access but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(0 == theFile.write_access_size(),
                          "expected 0 users with write access but found " << theFile.write_access_size());

    // test random user
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx"), "Expected user xxxx to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxtt", "/x"), "Expected user xxtt to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxtt", paths), "Expected user xxtt to have read access ");

    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("yyyy"), "Expected user yyyy to have write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzz", "/y"), "Expected user zzzz to have write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzz", paths), "Expected user zzzz to have write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_empty_file) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles/empty.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    BOOST_REQUIRE_MESSAGE(0 == theFile.read_access_size(),
                          "expected 0 users with read access but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(0 == theFile.write_access_size(),
                          "expected 0 users with write access but found " << theFile.write_access_size());

    // test random user
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx"), "Expected user xxxx to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxtt", "/x"), "Expected user xxtt to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxtt", paths), "Expected user xxtt to have read access ");

    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("yyyy"), "Expected user yyyy to have write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzz", "/y"), "Expected user zzzz to have write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzz", paths), "Expected user zzzz to have write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles/good1.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    // make sure we find all the users and the access right are correct
    // 	uid1    # a comment
    // 	uid2  # a comment
    // 	cog   # a comment
    //
    // 	#
    // 	# Read only uisers
    // 	#
    // 	-fred  # a comment
    // 	-bill # a comment
    // 	-jake # a comment

    // 	avi   /suite/write
    // 	-avi  /suite/read
    std::vector<std::string> expected_users_with_read_access;
    expected_users_with_read_access.emplace_back("fred");
    expected_users_with_read_access.emplace_back("bill");
    expected_users_with_read_access.emplace_back("jake");

    expected_users_with_read_access.emplace_back("uid1"); // users with write access also have read access
    expected_users_with_read_access.emplace_back("uid2"); // users with write access also have read access
    expected_users_with_read_access.emplace_back("cog");  // users with write access also have read access

    std::vector<std::string> expected_users_with_read_write_access;
    expected_users_with_read_write_access.emplace_back("uid1");
    expected_users_with_read_write_access.emplace_back("uid2");
    expected_users_with_read_write_access.emplace_back("cog");

    std::vector<std::string> expected_users_with_read_access_to_paths_only;
    expected_users_with_read_access_to_paths_only.emplace_back("avi");

    std::vector<std::string> expected_users_with_write_access_to_paths_only;
    expected_users_with_write_access_to_paths_only.emplace_back("avi");

    BOOST_REQUIRE_MESSAGE(
        expected_users_with_read_access.size() + expected_users_with_read_access_to_paths_only.size() ==
            theFile.read_access_size(),
        " expected " << expected_users_with_read_access.size() + expected_users_with_write_access_to_paths_only.size()
                     << " users with read access but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(expected_users_with_read_write_access.size() +
                                  expected_users_with_write_access_to_paths_only.size() ==
                              theFile.write_access_size(),
                          " expected " << expected_users_with_read_write_access.size() +
                                              expected_users_with_write_access_to_paths_only.size()
                                       << " users with write access but found " << theFile.write_access_size());

    // Users who have read access, to all including paths
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    std::vector<std::string>::const_iterator i;
    for (i = expected_users_with_read_access.begin(); i != expected_users_with_read_access.end(); ++i) {
        BOOST_CHECK_MESSAGE(theFile.verify_read_access(*i), "Expected user " << *i << " to have read access ");
        BOOST_CHECK_MESSAGE(theFile.verify_read_access(*i, "/x"), "Expected user " << *i << " to have read access ");
        BOOST_CHECK_MESSAGE(theFile.verify_read_access(*i, paths), "Expected user " << *i << " to have read access ");
    }
    for (i = expected_users_with_read_write_access.begin(); i != expected_users_with_read_write_access.end(); ++i) {
        BOOST_CHECK_MESSAGE(theFile.verify_write_access(*i), "Expected user " << *i << " to have write access ");
        BOOST_CHECK_MESSAGE(theFile.verify_write_access(*i, "/x"), "Expected user " << *i << " to have write access ");
        BOOST_CHECK_MESSAGE(theFile.verify_write_access(*i, paths), "Expected user " << *i << " to have write access ");
    }

    // Users who have restricted read/write access to certain paths only.
    std::vector<std::string> read_paths;
    read_paths.emplace_back("/suite/read");
    read_paths.emplace_back("/suite/read/f1");
    read_paths.emplace_back("/suite/read/f1/t1");
    for (i = expected_users_with_read_access_to_paths_only.begin();
         i != expected_users_with_read_access_to_paths_only.end();
         ++i) {
        BOOST_CHECK_MESSAGE(theFile.verify_read_access(*i), "Expected user " << *i << " to have read access ");
        BOOST_CHECK_MESSAGE(theFile.verify_read_access(*i, read_paths),
                            "Expected user " << *i << " to have read access to paths");
        BOOST_CHECK_MESSAGE(!theFile.verify_read_access(*i, paths),
                            "Expected user " << *i << " to NOT have read access to paths /a, /b , /c");
        BOOST_CHECK_MESSAGE(!theFile.verify_read_access(*i, "/suite"),
                            "Expected user " << *i << " to NOT have read access to paths /suite only /suite/read");
        BOOST_CHECK_MESSAGE(!theFile.verify_read_access(*i, "/suite/read_me"),
                            "Expected user "
                                << *i << " with path '/suite/read_me' to NOT have read access to path /suite/read");
    }

    std::vector<std::string> write_paths;
    write_paths.emplace_back("/suite/write");
    write_paths.emplace_back("/suite/write/f1");
    write_paths.emplace_back("/suite/write/f1/t1");
    for (i = expected_users_with_write_access_to_paths_only.begin();
         i != expected_users_with_write_access_to_paths_only.end();
         ++i) {
        BOOST_CHECK_MESSAGE(!theFile.verify_write_access(*i),
                            "Expected user " << *i << " to NOT have generic write access ");
        BOOST_CHECK_MESSAGE(theFile.verify_write_access(*i, write_paths),
                            "Expected user " << *i << " to have write access to paths");
        BOOST_CHECK_MESSAGE(!theFile.verify_write_access(*i, paths),
                            "Expected user " << *i << " to have NOT has write access to paths /a, /b , /c");
        BOOST_CHECK_MESSAGE(!theFile.verify_write_access(*i, "/suite"),
                            "Expected user " << *i << " to NOT have write access to paths /suite only /suite/write");
        BOOST_CHECK_MESSAGE(!theFile.verify_write_access(*i, "/suite/write_me"),
                            "Expected user "
                                << *i << " with path '/suite/write_me' to NOT have write access to paths /suite/write");
    }

    // test random user
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("xxxx"), "Expected user xxxx to NOT have read access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("xxxx", "/x"), "Expected user xxxx to NOT have read access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("xxxx", paths), "Expected user xxxx to NOT have read access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("*"), "Expected user *  to NOT have read access ");

    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("yyyy"), "Expected user yyyy  to NOT have write access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("zzzz", "/x"), "Expected user zzzz  to NOT have write access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("zzzz", paths), "Expected user zzzz  to NOT have write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_all_users_have_read_access) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles/all_read_access.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    // make sure we find all the users and the access right are correct
    // # These user have read and write access to the server
    // uid1  # a comment
    // uid2  # a comment
    // cog   # a comment
    //
    //
    // # Read only users
    //-*
    //-fred  # a comment
    //-bill # a comment
    //-jake # a comment

    std::vector<std::string> expected_users_with_read_write_access;
    expected_users_with_read_write_access.emplace_back("uid1");
    expected_users_with_read_write_access.emplace_back("uid2");
    expected_users_with_read_write_access.emplace_back("cog");

    // When all users have read access, the read access size should be empty
    BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0, " expected 0  but found " << theFile.read_access_size());

    BOOST_REQUIRE_MESSAGE(expected_users_with_read_write_access.size() == theFile.write_access_size(),
                          " expected " << expected_users_with_read_write_access.size()
                                       << " users with write access but found " << theFile.write_access_size());

    // Any user should have read access
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", "/x"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("bill"), "Expected user bill  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx"), "Expected user xxxx  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("uid1"), "Expected user xxxx  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("uid2"), "Expected user xxxx  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("cog"), "Expected user xxxx  to have read access ");

    std::vector<std::string>::const_iterator i;
    for (i = expected_users_with_read_write_access.begin(); i != expected_users_with_read_write_access.end(); ++i) {
        BOOST_REQUIRE_MESSAGE(theFile.verify_write_access(*i), "Expected user " << *i << " to have write access ");
        BOOST_REQUIRE_MESSAGE(theFile.verify_write_access(*i, "/x"),
                              "Expected user " << *i << " to have write access ");
        BOOST_REQUIRE_MESSAGE(theFile.verify_write_access(*i, paths),
                              "Expected user " << *i << " to have write access ");
    }

    // test random user for write access
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("yyyy"), "Expected user yyyy  to NOT have write access ");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("zzzz"), "Expected user zzzz  to NOT have write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_all_users_have_write_access) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles/all_write_access.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    // make sure we find all the users and the access right are correct
    //   uid1  # a comment
    //   uid2  # a comment
    //   cog   # a comment
    //   *
    //
    //   # Read only users
    //   -*
    //   -fred  # a comment
    //   -bill # a comment
    //   -jake # a comment

    // When all users have read access, the read access size should be empty
    BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0, " expected 0  but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(theFile.write_access_size() == 0, " expected 0  but found " << theFile.read_access_size());

    // Any user should have read access
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", "/x"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("bill"), "Expected user bill  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx"), "Expected user xxxx  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx", "/x"), "Expected user xxxx  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx", paths), "Expected user xxxx  to have read access ");

    // Any user should have read write access
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred"), "Expected user fred  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzzzz"), "Expected user zzzzzz  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzzzz", "/x"),
                          "Expected user zzzzzz  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("zzzzzz", paths),
                          "Expected user zzzzzz  to have read/write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_all_path_users_have_write_access) {
    ECF_NAME_THIS_TEST();

    std::string path =
        File::test_data("libs/core/test/data/goodWhiteListFiles/all_path_write_access.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    //   4.4.14
    //   * /  # same as '*'

    // When all users have read/write access, the read/write access size should be empty
    BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0, " expected 0  but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(theFile.write_access_size() == 0, " expected 0  but found " << theFile.write_access_size());

    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    std::vector<std::string> paths1;
    paths1.emplace_back("/a");
    paths1.emplace_back("/b");
    // Any user should have read/write access
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", "/x"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths1), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred"), "Expected user fred  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred", "/x"), "Expected user fred  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred", paths), "Expected user fred  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred", paths1),
                          "Expected user fred  to have read/write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_all_path_users_have_read_access) {
    ECF_NAME_THIS_TEST();

    std::string path =
        File::test_data("libs/core/test/data/goodWhiteListFiles/all_path_read_access.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    //   4.4.14
    //   -* /  # same as '*'

    // When all users have read/write access, the size should be empty
    BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 0, " expected 0  but found " << theFile.read_access_size());
    BOOST_REQUIRE_MESSAGE(theFile.write_access_size() == 0, " expected 0  but found " << theFile.write_access_size());

    // When no write access specified all user have write access
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    std::vector<std::string> paths1;
    paths1.emplace_back("/a");
    paths1.emplace_back("/b");
    // Any user should have read/write access
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred"), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred"), "Expected user fred  to have read access ");

    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("fred", paths1), "Expected user fred  to have read access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred", paths), "Expected user fred  to have read/write access ");
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("fred", paths1),
                          "Expected user fred  to have read/write access ");
}

BOOST_AUTO_TEST_CASE(test_white_list_path_access_list) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/core/test/data/goodWhiteListFiles/path_access.lists", "libs/core");

    WhiteListFile theFile;
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(theFile.load(path, false, errorMsg), "Failed to parse file " << path << "\n" << errorMsg);

    //   4.4.14
    //
    //   user1 /a,/b,/c  # user1 has read write access to suite /a /b /c
    //   user2 /a
    //   user2 /b
    //   user2 /c       # user2 has read write access to suite /a /b /c
    //   user3 /a /b /c # user3 has read write access to suite /a /b /c
    //
    //   /a /b /c userx # userx has read write access to suite /a /b /c
    //   /a,/b,/c usery # userx has read write access to suite /a /b /c
    //
    //   /a userz
    //   /b userz
    //   /c userz
    //
    //   -user4 /a,/b,/c  # user4 has read access to suite /a /b /c
    //   -user5 /a
    //   -user5 /b
    //   -user5 /c    # user5 has read access to suite /a /b /c
    //   -user6 /a /b /c   # user6 has read access to suite /a /b /c
    //
    //   /a /b /c -userxx # userxx has read  access to suite /a /b /c
    //   /a,/b,/c -useryy # userxy has read  access to suite /a /b /c
    //
    //   /a -userzz
    //   /b -userzz
    //   /c -userzz
    //
    // ##################################
    //*  /x /y   # all user have read/write access  suites /x /y
    //-* /w /z   # all user have read access to suite /w /z

    // When all users have read access, the read access size should be empty
    BOOST_REQUIRE_MESSAGE(theFile.read_access_size() == 7,
                          " expected 7  but found " << theFile.read_access_size() << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.write_access_size() == 7,
                          " expected 7  but found " << theFile.write_access_size() << theFile.dump_valid_users());

    // When no write access specified all user have write access
    std::vector<std::string> paths;
    paths.emplace_back("/a");
    paths.emplace_back("/b");
    paths.emplace_back("/c");
    std::vector<std::string> partial_paths;
    partial_paths.emplace_back("/a");
    partial_paths.emplace_back("/b");

    // read/write
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user1", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user1", partial_paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user1", paths),
                          "Expected user to have read/write access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user1", partial_paths),
                          "Expected user to have read/write access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user2", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user2", paths),
                          "Expected user to have read/write access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user3", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user3", paths),
                          "Expected user to have read/write access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userx", paths),
                          "Expected userx to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("userx", paths),
                          "Expected userx to have read/write access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("usery", paths),
                          "Expected userx to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("usery", paths),
                          "Expected userx to have read/write access to /a,/b,/c" << theFile.dump_valid_users());

    // read only
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user4", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user5", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user6", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userxx", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("useryy", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userzz", paths),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());

    // single path, read/write
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user1", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user1", "/a"),
                          "Expected user to have read/write access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user2", "/a"),
                          "Expected user to have read access to /a,/b,/c" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user2", "/a"),
                          "Expected user to have read/write access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user3", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("user3", "/a"),
                          "Expected user to have read/write access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userx", "/a"),
                          "Expected userx to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("userx", "/a"),
                          "Expected userx to have read/write access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("usery", "/a"),
                          "Expected userx to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("usery", "/a"),
                          "Expected userx to have read/write access to /a" << theFile.dump_valid_users());

    // single path, read
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user4", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user5", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user6", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userxx", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("useryy", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("userzz", "/a"),
                          "Expected user to have read access to /a" << theFile.dump_valid_users());

    // ============================================================================================
    // test * user, * means all users
    //*  /x /y   # all user have read/write access  suites /x /y
    //-* /w /z   # all user have read access to suite /w /z
    std::vector<std::string> single_pathx;
    single_pathx.emplace_back("/x");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx"),
                          "Expected *ALL* user to have read access" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx", single_pathx),
                          "Expected *ALL* user to have read access to /x" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("xxxx", single_pathx),
                          "Expected *ALL* user to have read/write access to /x" << theFile.dump_valid_users());

    std::vector<std::string> single_pathw;
    single_pathw.emplace_back("/w");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx", single_pathw),
                          "Expected *ALL* user to have read access to /w" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("xxxx", single_pathw),
                          "Expected failure for write access to /w" << theFile.dump_valid_users());

    std::vector<std::string> multiple_pathsxy;
    multiple_pathsxy.emplace_back("/x");
    multiple_pathsxy.emplace_back("/y");
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("xxxx", multiple_pathsxy),
                          "Expected *ALL* user to have read access to /x,/y" << theFile.dump_valid_users());

    // ============================================================================================
    // Failure
    // ============================================================================================

    // single path failure
    //   user1 /a,/b,/c  # user1 has read write access to suite /a /b /c
    std::vector<std::string> single_path_failure;
    single_path_failure.emplace_back("/fail");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("user1", "/fail"),
                          "Expected user to not have read access to /fail" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("user1", single_path_failure),
                          "Expected user to not have read access to /fail" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1", "/fail"),
                          "Expected user to not have read/write access to /fail" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1", single_path_failure),
                          "Expected user to not have read/write access to /fail" << theFile.dump_valid_users());

    // multi path failure
    //   user1 /a,/b,/c  # user1 has read write access to suite /a /b /c
    std::vector<std::string> multiple_paths_failure;
    multiple_paths_failure.emplace_back("/a");
    multiple_paths_failure.emplace_back("/b");
    multiple_paths_failure.emplace_back("/fail");
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("user1", multiple_paths_failure),
                          "Expected user to not have read access to /fail\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1", multiple_paths_failure),
                          "Expected user to not have read/write access to /fail\n"
                              << theFile.dump_valid_users());

    // Presence of *, should allow read access to all including unknown users
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("unknown"),
                          "Expected unknown user to pass due to presence of *\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("unknown", "/x"),
                          "Expected pass \n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("unknown", "/y"), "Expected pass\n" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("unknown", "/w"), "Expected pass\n" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("unknown", "/z"), "Expected pass\n" << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("unknown", "/x"),
                          "Expected pass\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(theFile.verify_write_access("unknown", "/y"),
                          "Expected pass\n"
                              << theFile.dump_valid_users());

    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("unknown", "/a"),
                          "Expected failure for unknown user for path /a\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("unknown", "/a"),
                          "Expected failure for unknown user for path /a\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("unknown", paths),
                          "Expected failure for unknown user\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("unknown", paths),
                          "Expected failure for unknown user\n"
                              << theFile.dump_valid_users());

    // user1 /a,/b,/c  # user1 has read write access to suite /a /b /c
    BOOST_REQUIRE_MESSAGE(theFile.verify_read_access("user1"),
                          "Expected user1 to not have read access, EVEN if no paths specified, --news has no paths\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("user1", std::vector<std::string>()),
                          "Expected user to not have read access if no paths specified\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_read_access("user1", ""),
                          "Expected user to not have read access if no paths specified\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1"),
                          "Expected user to not have read/write if no paths specified\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1", ""),
                          "Expected user to not have read/write if no paths specified\n"
                              << theFile.dump_valid_users());
    BOOST_REQUIRE_MESSAGE(!theFile.verify_write_access("user1", std::vector<std::string>()),
                          "Expected user to not have read/write if no paths specified\n"
                              << theFile.dump_valid_users());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
