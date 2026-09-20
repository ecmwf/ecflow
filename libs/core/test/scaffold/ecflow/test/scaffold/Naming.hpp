// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_test_scaffold_Naming_HPP
#define ecflow_test_scaffold_Naming_HPP

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

namespace ecf::test::scaffold {

inline std::string name_this_test() {
    std::string fullname = boost::unit_test::framework::current_test_case().p_name;
    long parent_id       = boost::unit_test::framework::current_test_case().p_parent_id;
    long master_id       = boost::unit_test::framework::master_test_suite().p_id;

    while (parent_id != master_id) {
        const auto& parent = boost::unit_test::framework::get<boost::unit_test::test_suite>(parent_id);
        fullname           = std::string(parent.p_name) + std::string(" / ") + fullname;
        parent_id          = parent.p_parent_id;
    }
    return fullname;
}

///
/// @brief Describes the running test process and, when available, the Boost.Test unit currently executing.
///
/// The description is a sequence of "key: value" lines, intended for artefacts left on the filesystem
/// (such as lock files), so that a leftover artefact can be traced back to the test that created it.
/// The following keys are always present: created (UTC time stamp), host, pid, cwd, executable.
/// When a test case is executing, the following keys are also present: test (the full path of the
/// test case, as accepted by --run_test), test_id (the Boost.Test unit identifier), and location
/// (the source file and line where the test case is declared). Outside any test case (for example,
/// in a global fixture), only the test key is present, naming the test module.
///
/// @return The description, with each line terminated by a newline
///
inline std::string describe_current_test() {
    namespace but = boost::unit_test;

    std::ostringstream out;

    {
        auto now   = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm = {};
        gmtime_r(&now, &tm);
        out << "created: " << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ") << '\n';
    }
    {
        char hostname[256] = {};
        if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
            std::strcpy(hostname, "unknown");
        }
        out << "host: " << hostname << '\n';
    }
    out << "pid: " << getpid() << '\n';
    {
        std::error_code ec;
        auto cwd = std::filesystem::current_path(ec);
        out << "cwd: " << (ec ? std::string{"unknown"} : cwd.string()) << '\n';
    }
    {
        const auto& master = but::framework::master_test_suite();
        out << "executable: " << (master.argc > 0 ? master.argv[0] : "unknown") << '\n';
    }
    if (auto id = but::framework::current_test_case_id(); id != but::INV_TEST_UNIT_ID) {
        const auto& unit = but::framework::get(id, but::TUT_ANY);
        if (id == but::framework::master_test_suite().p_id) {
            // Outside any test case (e.g. in a global fixture), the current unit is the test module itself
            out << "test: " << unit.full_name() << " (test module; outside any test case)" << '\n';
        }
        else {
            out << "test: " << unit.full_name() << '\n';
            out << "test_id: " << id << '\n';
            out << "location: " << unit.p_file_name << ':' << unit.p_line_num << '\n';
        }
    }

    return out.str();
}

} // namespace ecf::test::scaffold

// NOLINTBEGIN(bugprone-macro-parentheses)
#define ECF_NAME_THIS_TEST(ARGS)                                                       \
    do {                                                                               \
        std::cout << " * " << ecf::test::scaffold::name_this_test() ARGS << std::endl; \
    } while (0)

#define ECF_TEST_DBG(ARGS)                         \
    do {                                           \
        std::cout << " +++ " << ARGS << std::endl; \
    } while (0)

#define ECF_TEST_ERR(ARGS)                         \
    do {                                           \
        std::cerr << " +++ " << ARGS << std::endl; \
    } while (0)
// NOLINTEND(bugprone-macro-parentheses)

#endif /* ecflow_test_scaffold_Naming_HPP */
