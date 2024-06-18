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

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_MirrorAttr)

BOOST_AUTO_TEST_CASE(can_parse_mirror_attribute_on_task_with_default_parameters) {
    std::cout << "..." << boost::unit_test::framework::current_test_unit().full_name() << std::endl;

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              mirror --name A --remote_path /s1/f1/t2
          endfamily
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_CHECK_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);

    const auto& suites = defs.suiteVec();
    BOOST_CHECK_EQUAL(suites.size(), static_cast<size_t>(1));

    const auto& families = suites[0]->familyVec();
    BOOST_CHECK_EQUAL(families.size(), static_cast<size_t>(1));

    const auto& tasks = families[0]->taskVec();
    BOOST_CHECK_EQUAL(tasks.size(), static_cast<size_t>(1));

    const auto& mirrors = tasks[0]->mirrors();
    BOOST_CHECK_EQUAL(mirrors.size(), static_cast<size_t>(1));

    const auto& mirror = mirrors[0];
    BOOST_CHECK_EQUAL(mirror.name(), "A");
    BOOST_CHECK_EQUAL(mirror.remote_path(), "/s1/f1/t2");
    BOOST_CHECK_EQUAL(mirror.remote_host(), "%ECF_MIRROR_REMOTE_HOST%");
    BOOST_CHECK_EQUAL(mirror.remote_port(), "%ECF_MIRROR_REMOTE_PORT%");
    BOOST_CHECK_EQUAL(mirror.polling(), "%ECF_MIRROR_REMOTE_POLLING%");
    BOOST_CHECK_EQUAL(mirror.ssl(), false);
}

BOOST_AUTO_TEST_CASE(can_parse_mirror_attribute_on_task_with_all_attributes) {
    std::cout << "..." << boost::unit_test::framework::current_test_unit().full_name() << std::endl;

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              mirror --name A --remote_path /s1/f1/t2 --remote_host hostname --remote_port 1234 --polling 20 --ssl
          endfamily
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_CHECK_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);

    const auto& suites = defs.suiteVec();
    BOOST_CHECK_EQUAL(suites.size(), static_cast<size_t>(1));

    const auto& families = suites[0]->familyVec();
    BOOST_CHECK_EQUAL(families.size(), static_cast<size_t>(1));

    const auto& tasks = families[0]->taskVec();
    BOOST_CHECK_EQUAL(tasks.size(), static_cast<size_t>(1));

    const auto& mirrors = tasks[0]->mirrors();
    BOOST_CHECK_EQUAL(mirrors.size(), static_cast<size_t>(1));

    const auto& mirror = mirrors[0];
    BOOST_CHECK_EQUAL(mirror.name(), "A");
    BOOST_CHECK_EQUAL(mirror.remote_path(), "/s1/f1/t2");
    BOOST_CHECK_EQUAL(mirror.remote_host(), "hostname");
    BOOST_CHECK_EQUAL(mirror.remote_port(), "1234");
    BOOST_CHECK_EQUAL(mirror.polling(), "20");
    BOOST_CHECK_EQUAL(mirror.ssl(), true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
