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
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

class Run {
public:
    using clock_t    = std::chrono::high_resolution_clock;
    using instant_t  = std::chrono::time_point<clock_t>;
    using duration_t = std::chrono::microseconds;

    void start() { start_ = std::chrono::high_resolution_clock::now(); }
    void finish() { finish_ = std::chrono::high_resolution_clock::now(); }

    duration_t elapsed() const { return std::chrono::duration_cast<duration_t>(finish_ - start_); }

    static void summary(const Run& expected, const Run& actual) {
        auto expected_time = expected.elapsed().count();
        auto actual_time   = actual.elapsed().count();
        std::cout << "Expected time: " << expected_time << " ms" << std::endl;
        std::cout << "  Actual time: " << actual_time << " ms" << std::endl;
        auto difference = expected_time - actual_time;
        std::cout << "   Difference: " << difference << " ms [" << (difference < 0 ? "🔴" : "🟢") << "]" << std::endl;
    }

private:
    instant_t start_{};
    instant_t finish_{};
};

struct RunMeasure
{
public:
    RunMeasure(Run& run) : run_(run) { run_.start(); }

    ~RunMeasure() { run_.finish(); }

private:
    Run& run_;
};

std::vector<std::string> split_lines(const std::string& input) {
    std::vector<std::string> lines;
    boost::split(lines, input, boost::is_any_of("\n"));
    return lines;
}

void diff(const std::string& expected, const std::string& actual) {

    auto expected_lines = split_lines(expected);
    auto actual_lines   = split_lines(actual);

    if (expected_lines.size() != actual_lines.size()) {
        std::cout << "Number of lines differ: expected " << expected_lines.size() << ", actual " << actual_lines.size()
                  << std::endl;
        BOOST_CHECK(expected_lines.size() == actual_lines.size());
    }

    for (size_t i = 0; i < std::max(expected_lines.size(), actual_lines.size()); ++i) {
        if (i < expected_lines.size() && i < actual_lines.size() && expected_lines[i] != actual_lines[i]) {

            std::cout << "Difference at line " << i + 1 << std::endl;
            std::cout << "  Expected: >" << expected_lines[i] << "<" << std::endl;
            std::cout << "    Actual: >" << actual_lines[i] << "<" << std::endl;

            std::cout << " Full Expected:" << std::endl << expected << std::endl;
            std::cout << "   Full Actual:" << std::endl << actual << std::endl;

            BOOST_CHECK(expected_lines[i] == actual_lines[i]);
            return;
        }
    }
}

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_WriterDefs)

static std::vector<std::string> defs_files = {
    // ecf::File::test_data("libs/node/test/parser/data/single_defs/mega.def", "parser"),

    ecf::File::test_data("libs/node/test/parser/data/good_defs_state/defs/defs_state.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autoarchive/autoarchive.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autoarchive/autoarchive2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autocancel/autocancel.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autorestore/autorestore_test.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autorestore/autorestore.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/autorestore/autorestore2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/aviso/aviso.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock3.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock4.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/clock/clock5.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/comment/comment.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/complete/complete.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/complete/complex.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/complete/why.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/cron/cron_simple.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/cron/cron.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/cron/cron1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/cron/cron2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/cron/last.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/date/date.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/day/day.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/defstatus/defstatus.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/edit/edit.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/event_1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/event_2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/event_set.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/family_event.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/spaces.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/event/suite_event.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/extern/extern.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/extern/first.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/extern/plain.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/extern/second.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/extern/simple_extern.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/family/hierarchical_family.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/family/missingEnds.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/family/simple_family.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/generic/generic.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/JIRA/ecflow_1550.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/JIRA/ecflow_337_.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/JIRA/ecflow_337.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/label/label.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/label/multi_line_lables.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/label/semicolon.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/label/spaces.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/late/late.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/lifecycle.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/basic.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/inlimit_all_types.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/inlimit_family.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/inlimit_only.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/limit_check.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/limit.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/limit2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/limit3.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/sub_only.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/limit/sub_only1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/meter/negative.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/meter/simple_meter.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/meter/spaces.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/mirror/mirror.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/queue/queue_string.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/queue/queue.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/queue/queue2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_date_list.ecf", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_date.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_datetime.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_day.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_enumerate_quotes.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_enumerated.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_file.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_integer_1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_integer_2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_integer.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_string_quotes.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/repeat/repeat_string.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/suite/multi_statements_per_line.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/suite/multi_suite.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/suite/simple_suite.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/suite/suite_with_hierarchy.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/suite/suite_with_task.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/task/simple_task.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/task/spaces.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/task/task.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/time/time_1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/time/time_2.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/time/time.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/today/today.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/today/today1.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/all_trigger_examples.def", "parser"), // #problem
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/anded_ored.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/complex_hier.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/complex_trigger.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/ECFLOW_1442.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/ECFLOW_867.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/extension.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/late.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/limit.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/relative.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/simple_trigger.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/trigger/trigger_references.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/variable/alias.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/variable/duplicate.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/variable/variable.txt", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/verify/verify.def", "parser"),
    ecf::File::test_data("libs/node/test/parser/data/good_defs/zombie/zombie.def", "parser")};

void test_write_using_style(const PrintStyle::Type_t& style) {
    ECF_NAME_THIS_TEST();

    // Check Defs written using STATE style

    for (auto f : defs_files) {

        BOOST_TEST_MESSAGE("Testing using file: " + f);

        Defs defs;
        defs.restore(f);

        std::string expected;
        Run run_expected;
        {
            RunMeasure measure(run_expected);
            defs.save_as_string(expected, style);
        }

        std::string actual;
        Run run_actual;
        {
            RunMeasure measure(run_actual);
            auto ctx = ecf::Context::make_for(style);
            ecf::write_t(actual, defs, ctx);
        }

        Run::summary(run_expected, run_actual);

        diff(expected, actual);
    }
}

BOOST_AUTO_TEST_CASE(test_write_simple_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    auto s  = defs.add_suite("s");
    auto f1 = s->add_family("f1");
    auto t1 = f1->add_task("t1");
    t1->add_variable("name", "value");
    auto a1 = t1->add_alias("a1");

    auto style = PrintStyle::DEFS;

    std::string expected;
    { defs.save_as_string(expected, style); }

    std::string actual;
    {
        auto ctx = ecf::Context::make_for(style);
        ecf::write_t(actual, defs, ctx);
    }

    BOOST_CHECK(expected == actual);
}

BOOST_AUTO_TEST_CASE(test_write_using_defs_style) {
    ECF_NAME_THIS_TEST();

    // Check Defs written using DEFS style
    test_write_using_style(PrintStyle::DEFS);
}

BOOST_AUTO_TEST_CASE(test_write_using_state_style) {
    ECF_NAME_THIS_TEST();

    // Check Defs written using STATE style
    test_write_using_style(PrintStyle::STATE);
}

BOOST_AUTO_TEST_CASE(test_write_using_net_style) {
    ECF_NAME_THIS_TEST();

    // Check Defs written using NET style
    test_write_using_style(PrintStyle::NET);
}

BOOST_AUTO_TEST_CASE(test_write_using_migrate_style) {
    ECF_NAME_THIS_TEST();

    // Check Defs written using MIGRATE style
    test_write_using_style(PrintStyle::MIGRATE);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
