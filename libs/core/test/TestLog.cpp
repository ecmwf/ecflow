/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;
using namespace boost;

void dump_path(const fs::path& path) {
    ECF_TEST_DBG(<< "path = " << path);
    ECF_TEST_DBG(<< "path.root_path(): " << path.root_path());
    ECF_TEST_DBG(<< "path.root_name() : " << path.root_name());
    ECF_TEST_DBG(<< "path.root_directory()  : " << path.root_directory());
    ECF_TEST_DBG(<< "path.relative_path()  : " << path.relative_path());
    ECF_TEST_DBG(<< "path.parent_path()  : " << path.parent_path());
    ECF_TEST_DBG(<< "path.filename()  : " << path.filename());
    ECF_TEST_DBG(<< "path.stem()  : " << path.stem());
    ECF_TEST_DBG(<< "path.extension()  : " << path.extension());
}

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Log)

static std::string getLogPath() {

    // ECFLOW-712, generate unique name for log file, To allow parallel test
    std::string log_file = "libs/core/test/logfile";
    log_file += Pid::getpid(); // can throw
    log_file += ".txt";
    return File::test_data(log_file, "libs/core");
}

BOOST_AUTO_TEST_CASE(test_log) {
    std::string path = getLogPath();
    ECF_NAME_THIS_TEST(<< ", using log file:" << path);

    // delete the log file if it exists.
    fs::remove(path);
    BOOST_REQUIRE_MESSAGE(!fs::exists(path), "log file not deleted " << path);

    LogFlusher logFlusher;
    Log::create(path);
    LOG(Log::MSG, "First Message");
    LOG(Log::LOG, "LOG");
    LOG(Log::ERR, "ERROR");
    LOG(Log::WAR, "WARNING");
    LOG(Log::DBG, "DEBUG");
    LOG(Log::OTH, "OTHER");
    log(Log::OTH, "test: void log(Log::LogType,const std::string& message)");

    LOG(Log::OTH, "test: LOG(level,path << path) " << path << " " << path);

    Log::instance()->log(Log::OTH, "OTHER2");

    BOOST_CHECK_MESSAGE(fs::exists(path), "log file " << path << " not created \n");
}

BOOST_AUTO_TEST_CASE(test_log_append) {
    ECF_NAME_THIS_TEST();

    std::string path = getLogPath();

    BOOST_REQUIRE_MESSAGE(fs::exists(path), "log file " << path << " not created by previous test\n");

    {
        LogFlusher logFlusher;
        LOG(Log::MSG, "Last Message");
    }

    // Load the log file into a vector, of strings, and test content
    std::vector<std::string> lines;
    BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(path, lines, true /*IGNORE EMPTY LINE AT THE END*/),
                          "Failed to open log file" << " (" << strerror(errno) << ")");
    BOOST_REQUIRE(lines.size() != 0);
    BOOST_CHECK_MESSAGE(lines.size() == 10, " Expected 10 lines in log, but found " << lines.size() << "\n");
    BOOST_CHECK_MESSAGE(lines[0].find("First Message") != std::string::npos,
                        "Expected first line to contain 'First Message' but found " << lines[0] << "\n");
    BOOST_CHECK_MESSAGE(lines.back().find("Last Message") != std::string::npos,
                        "Expected last line to contain 'Last Message' but found " << lines.back() << "\n");

    // Clear the log file. Comment out for debugging
    Log::instance()->clear();
    BOOST_CHECK_MESSAGE(fs::file_size(path) == 0, "Clear of log file failed\n");

    // Remove the log file. Comment out for debugging
    fs::remove(path);

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_log_path) {
    ECF_NAME_THIS_TEST();

    Log::create("test_log_path.log");

    // make sure path returned is absolute
    std::string path = Log::instance()->path();
    BOOST_REQUIRE_MESSAGE(path[0] == '/', "Expected absolute paths for log file but found " << path);

    // Remove the log file. Comment out for debugging
    fs::remove(path);

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_log_new_path_errors) {
    ECF_NAME_THIS_TEST();

    // delete the log file if it exists.
    std::string path = getLogPath();
    fs::remove(path);

    // create a now log file.
    Log::create(path);
    LOG(Log::MSG, "First Message");
    LOG(Log::LOG, "LOG");

    // Specify bad paths for new log file
    // First test empty path throws
    BOOST_REQUIRE_THROW(Log::instance()->new_path(""), std::runtime_error);

    // If a path is specified make sure parent directory exists
    fs::path current_path = fs::current_path();
    std::string path2     = current_path.string();
    path2 += "/a/made/up/path/fred.log";
    BOOST_REQUIRE_THROW(Log::instance()->new_path(path2), std::runtime_error);

    // Make sure path does not correspond to a directory
    BOOST_REQUIRE_THROW(Log::instance()->new_path(current_path.parent_path().string()), std::runtime_error);

    // fs::path valid_path = getLogPath();
    // dump_path(valid_path);

    // Remove the log file. Comment out for debugging
    fs::remove(Log::instance()->path());

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_log_new_path) {
    ECF_NAME_THIS_TEST();

    // delete the log file if it exists.
    std::string path = getLogPath();
    fs::remove(path);

    // create a new log file.
    Log::create(path);
    BOOST_CHECK_MESSAGE(fs::exists(Log::instance()->path()),
                        "Log file should be created after explicit call to Log::create()\n");
    LOG(Log::LOG, "LOG");
    fs::remove(Log::instance()->path());

    // Specify a new log path. Path could be a relative path like "test/logfile.log"
    std::string relative_path = File::test_data("libs/core/test/logfile.log", "libs/core");

    BOOST_REQUIRE_NO_THROW(Log::instance()->new_path(relative_path));
    BOOST_CHECK_MESSAGE(!fs::exists(Log::instance()->path()),
                        "Log file should *NOT* be created until first message is logged\n");
    LOG(Log::LOG, "LOG");
    BOOST_CHECK_MESSAGE(fs::exists(Log::instance()->path()),
                        "Log file should be created after first message is logged\n");
    fs::remove(Log::instance()->path());

    // Specify a new log path. This time we just specify a file name, without a path.
    BOOST_REQUIRE_NO_THROW(Log::instance()->new_path("testlog.log"));
    BOOST_CHECK_MESSAGE(!fs::exists(Log::instance()->path()),
                        "Log file should not be created until first message is logged\n");
    // File not created until a message is logged
    LOG(Log::LOG, "LOG");
    BOOST_CHECK_MESSAGE(fs::exists(Log::instance()->path()),
                        "Log file should be created after first message is logged\n");

    fs::remove(Log::instance()->path());

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_get_last_n_lines_from_log) {
    ECF_NAME_THIS_TEST();

    // delete the log file if it exists.
    std::string path = getLogPath();
    fs::remove(path);
    BOOST_REQUIRE_MESSAGE(!fs::exists(path), "log file not deleted " << path);

    // Create the log file;
    Log::create(path);
    BOOST_CHECK_MESSAGE(fs::exists(path), "log file " << path << " not created \n");

    // Log file should be empty
    const int NO_OF_LINES_IN_LOG_FILE = 200;
    {
        for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
            std::string line = Log::instance()->contents(i);
            BOOST_CHECK_MESSAGE(line.empty(), "Expected empty string but found\n" << line);
        }
    }

    // Populate the log file
    std::string msg = "This is message ";
    for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)
        LOG(Log::MSG, msg << i);

    // Now check, getting the lines
    {
        std::string line = Log::instance()->contents(0);
        BOOST_CHECK_MESSAGE(line.empty(), "Expected empty string but found\n" << line);
    }
    {
        // Check we get back the number of line requested
        for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
            std::string lines = Log::instance()->contents(i);
            int newlineCount  = std::count(lines.begin(), lines.end(), '\n');
            BOOST_CHECK_MESSAGE(i == newlineCount, "expected to  " << i << " newlines but found " << newlineCount);
        }
    }
    {
        // Check we get back *ALL* lines requested
        std::string lines = Log::instance()->contents(NO_OF_LINES_IN_LOG_FILE);
        for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
            std::stringstream ss;
            ss << msg << i;
            std::string str_to_find = ss.str();
            BOOST_CHECK_MESSAGE(lines.find(str_to_find) != std::string::npos,
                                "expected to find " << str_to_find << " in the log file");
        }
    }

    {
        // Request more than is available, should only get back whats there
        std::string lines = Log::instance()->contents(NO_OF_LINES_IN_LOG_FILE * 2);
        int newlineCount  = std::count(lines.begin(), lines.end(), '\n');
        BOOST_CHECK_MESSAGE(NO_OF_LINES_IN_LOG_FILE == newlineCount,
                            "expected " << NO_OF_LINES_IN_LOG_FILE << " newlines but found " << newlineCount);
    }

    fs::remove(Log::instance()->path());

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_get_first_n_lines_from_log) {
    ECF_NAME_THIS_TEST();

    // delete the log file if it exists.
    std::string path = getLogPath();
    fs::remove(path);
    BOOST_REQUIRE_MESSAGE(!fs::exists(path), "log file not deleted " << path << " not created \n");

    // Create the log file;
    Log::create(path);
    BOOST_CHECK_MESSAGE(fs::exists(path), "log file " << path << " not created \n");

    // Populate the log file
    const int NO_OF_LINES_IN_LOG_FILE = 200;
    std::string msg                   = "This is message ";
    for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)
        LOG(Log::MSG, msg << i);

    // Now check, getting the lines
    {
        // Get the first line
        std::string line     = Log::instance()->contents(-1);
        std::string expected = msg + "0";
        BOOST_CHECK_MESSAGE(line.find(expected) != std::string::npos,
                            "Expected '" << expected << "' but found\n"
                                         << line);
    }
    {
        // Get the first & second line
        std::string line      = Log::instance()->contents(-2);
        std::string expected0 = msg + "0";
        std::string expected1 = msg + "1";
        BOOST_CHECK_MESSAGE(line.find(expected0) != std::string::npos,
                            "Expected '" << expected0 << "' but found\n"
                                         << line);
        BOOST_CHECK_MESSAGE(line.find(expected1) != std::string::npos,
                            "Expected '" << expected1 << "' but found\n"
                                         << line);
    }
    {
        // Check we get back the number of line requested
        for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
            std::string lines = Log::instance()->contents(-i);
            int newlineCount  = std::count(lines.begin(), lines.end(), '\n');
            BOOST_CHECK_MESSAGE(i == newlineCount, "expected to  " << i << " newlines but found " << newlineCount);
        }
    }
    {
        std::string lines = Log::instance()->contents(-NO_OF_LINES_IN_LOG_FILE);
        for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
            std::stringstream ss;
            ss << msg << i;
            std::string expected = ss.str();
            BOOST_CHECK_MESSAGE(lines.find(expected) != std::string::npos,
                                "Expected '" << expected << "' but found for i " << i);
        }
    }

    {
        // Request more than is available, should only get back whats there
        std::string lines = Log::instance()->contents(-NO_OF_LINES_IN_LOG_FILE * 2);
        int newlineCount  = std::count(lines.begin(), lines.end(), '\n');
        BOOST_CHECK_MESSAGE(NO_OF_LINES_IN_LOG_FILE == newlineCount,
                            "expected " << NO_OF_LINES_IN_LOG_FILE << " newlines but found " << newlineCount);
    }

    fs::remove(Log::instance()->path());

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_get_log_timing) {
    ECF_NAME_THIS_TEST();

    // *************************************************************************************
    // This test was used with *DIFFERENT* implementations for Log::instance()->contents(1)
    // What is shows, is that for optimal performance we should *NOT* load the entire log file
    // This can be several giga bytes.
    // **************************************************************************************

    // delete the log file if it exists.
    std::string path = getLogPath();
    fs::remove(getLogPath());
    BOOST_REQUIRE_MESSAGE(!fs::exists(path), "log file not deleted " << path << " not created \n");

    // Create the log file;
    Log::create(path);
    BOOST_CHECK_MESSAGE(fs::exists(path), "log file " << path << " not created \n");

    // Populate the log file
    const int NO_OF_LINES_IN_LOG_FILE = 20000;
    std::string msg                   = "This is message ";
    for (int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)
        LOG(Log::MSG, msg << i);

    DurationTimer timer;

    {
        const int LOOP = 100;
        for (int i = 0; i < LOOP; i++) {
            std::string lines = Log::instance()->contents(1);
            BOOST_CHECK_MESSAGE(!lines.empty(), "expected entry");
        }
    }

    fs::remove(Log::instance()->path());

    // Explicitly destroy log. To keep valgrind happy
    Log::destroy();

#if PRINT_TIMING_RESULTS
    ECF_TEST_DBG(timer.duration() << "s\n");
#endif
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
