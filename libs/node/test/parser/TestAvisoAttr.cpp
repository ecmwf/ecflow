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

#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_AvisoAttr)

BOOST_AUTO_TEST_CASE(can_parse_aviso_attribute_on_task_with_default_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              aviso --name A --listener '{ "event": "mars", "request": { "class": "od"} }'
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

    const auto& avisos = tasks[0]->avisos();
    BOOST_CHECK_EQUAL(avisos.size(), static_cast<size_t>(1));

    const auto& aviso = avisos[0];
    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "od"} }')");
    BOOST_CHECK_EQUAL(aviso.url(), "%ECF_AVISO_URL%");
    BOOST_CHECK_EQUAL(aviso.schema(), "%ECF_AVISO_SCHEMA%");
    BOOST_CHECK_EQUAL(aviso.auth(), "%ECF_AVISO_AUTH%");
    BOOST_CHECK_EQUAL(aviso.polling(), "%ECF_AVISO_POLLING%");
}

BOOST_AUTO_TEST_CASE(can_parse_aviso_attribute_on_task_with_all_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              aviso --name A --listener '{ "event": "mars", "request": { "class": "od"} }' --url http://host:port --schema /path/to/schema --auth /path/to/auth --polling 60 --reason 'this is a reason'
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

    const auto& avisos = tasks[0]->avisos();
    BOOST_CHECK_EQUAL(avisos.size(), static_cast<size_t>(1));

    const auto& aviso = avisos[0];
    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "od"} }')");
    BOOST_CHECK_EQUAL(aviso.url(), "http://host:port");
    BOOST_CHECK_EQUAL(aviso.schema(), "/path/to/schema");
    BOOST_CHECK_EQUAL(aviso.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(aviso.polling(), "60");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
