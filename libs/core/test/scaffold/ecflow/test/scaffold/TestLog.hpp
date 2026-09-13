/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_test_scaffold_TestLog_HPP
#define ecflow_test_scaffold_TestLog_HPP

#include <string>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Log.hpp"

namespace ecf::test::scaffold {

///
/// @brief Provides the Log singleton for the duration of a test, backed by a log file that is removed afterwards.
///
/// The server-side code assumes that the log is always present, so tests exercising it create an instance at the
/// start of the test case. The log is destroyed, and the log file removed, when the instance goes out of scope.
///
class TestLog {
public:
    ///
    /// @brief Creates the Log singleton, writing to the given file.
    ///
    /// @param[in] log_path The path of the log file
    ///
    explicit TestLog(const std::string& log_path)
        : log_path_(log_path) {
        Log::create(log_path_);
    }

    TestLog(const TestLog&)            = delete;
    TestLog& operator=(const TestLog&) = delete;

    ~TestLog() {
        // Remove the log file. Comment out for debugging
        fs::remove(log_path_);

        // Explicitly destroy log. To keep valgrind happy
        Log::destroy();
    }

private:
    std::string log_path_;
};

} // namespace ecf::test::scaffold

#endif /* ecflow_test_scaffold_TestLog_HPP */
