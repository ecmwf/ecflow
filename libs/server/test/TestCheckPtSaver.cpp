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

#include <boost/test/unit_test.hpp>

#include "ecflow/server/CheckPtSaver.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

/**
 * A self cleaning test file, useful to automate test data storage/cleanup, with automatic generation of file names.
 */
class TestFile {
public:
    /**
     * Create a handle for a 'non-existent' data file
     */
    explicit TestFile(const std::string& basename) : absolute_path_(TestFile::unique_absolute_path(basename)) {}

    /**
     * Create a handle for a data file, with the given content
     */
    TestFile(const std::string& basename, const std::string& content)
        : absolute_path_(TestFile::unique_absolute_path(basename)) {
        f_store(absolute_path_.string(), content);
    }
    ~TestFile() { fs::remove(absolute_path_); }

    [[nodiscard]] const fs::path& path() const { return absolute_path_; }
    [[nodiscard]] std::string load() const { return f_load(absolute_path_); }

    static fs::path unique_absolute_path(const std::string& basename) {
        return fs::absolute(ecf::fsx::unique_path(basename));
    }

private:
    static void f_store(const fs::path& outfile, const std::string& data) {
        std::ofstream ofs(outfile.string());
        ofs << data;
    }

    static std::string f_load(const fs::path& infile) {
        std::ifstream ifs(infile.string());
        std::string buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return buffer;
    }

private:
    fs::path absolute_path_;
};

void handle_store(const fs::path& temporary) {
    std::ofstream ofs(temporary.string());
    ofs << "2";
}

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_CheckPtSaver)

BOOST_AUTO_TEST_CASE(test_checkpt_store_successful_case0) {
    ECF_NAME_THIS_TEST();

    // Case 0: neither current and backup exist

    TestFile current("current_%%%%-%%%%-%%%%"); // Important: no actual file is created
    TestFile backup("backup_%%%%-%%%%-%%%%");   // Important: no actual file is created

    CheckPtSaver::storeWithBackup(current.path(), backup.path(), handle_store);

    BOOST_CHECK_EQUAL(current.load(), "2");
    BOOST_CHECK(!fs::exists(backup.path()));
}

BOOST_AUTO_TEST_CASE(test_checkpt_store_successful_case1) {
    ECF_NAME_THIS_TEST();

    // Case 1: current and backup both exist and are non-empty

    TestFile current("current_%%%%-%%%%-%%%%", "1");
    TestFile backup("backup_%%%%-%%%%-%%%%", "X");

    CheckPtSaver::storeWithBackup(current.path(), backup.path(), handle_store);

    BOOST_CHECK_EQUAL(current.load(), "2");
    BOOST_CHECK_EQUAL(backup.load(), "1");
}

BOOST_AUTO_TEST_CASE(test_checkpt_store_successful_case2) {
    ECF_NAME_THIS_TEST();

    // Case 2: current and backup both exist, and current is empty

    TestFile current("current_%%%%-%%%%-%%%%", ""); // Important: file is created empty
    TestFile backup("backup_%%%%-%%%%-%%%%", "X");

    CheckPtSaver::storeWithBackup(current.path(), backup.path(), handle_store);

    BOOST_CHECK_EQUAL(current.load(), "2");
    BOOST_CHECK_EQUAL(backup.load(), "X");
}

BOOST_AUTO_TEST_CASE(test_checkpt_store_successful_case3) {
    ECF_NAME_THIS_TEST();

    // Case 3: current exists, but no backup is available

    TestFile current("current_%%%%-%%%%-%%%%", "1");
    TestFile backup("backup_%%%%-%%%%-%%%%"); // Important: no actual file is created

    CheckPtSaver::storeWithBackup(current.path(), backup.path(), handle_store);

    BOOST_CHECK_EQUAL(current.load(), "2");
    BOOST_CHECK_EQUAL(backup.load(), "1");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
