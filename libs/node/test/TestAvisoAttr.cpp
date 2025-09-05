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
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_AvisoAttr)

BOOST_AUTO_TEST_CASE(can_run_aviso_attribute_with_variable_substitution) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithTestFile schema(NamedTestFile{"schema.aviso.json"}, R"(
      {
        "version":0.1,
        "payload":"location",
        "mars":{
          "endpoint":[
            {
              "engine":[
                "etcd_rest",
                "etcd_grpc"
              ],
              "base":"/ec/mars",
              "stem":"date={date},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}"
            },
            {
              "engine":[
                "file_based"
              ],
              "base":"/tmp/aviso/mars",
              "stem":"{class}/{expver}/{domain}/{date}/{time}/{stream}/{step}"
            }
          ]
        }
      })");

    std::string definition = R"(
        suite s1
          family f1
            edit CLASS 'od'
            edit ECF_AVISO_URL 'https://example.com/aviso'
            edit ECF_AVISO_SCHEMA 'schema.aviso.json'
            edit ECF_AVISO_AUTH ''
            edit ECF_AVISO_POLLING '30'
            task t1
              aviso --name A --listener '{ "event": "mars", "request": { "class": "%CLASS%" } }'
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
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "%CLASS%" } }')");
    BOOST_CHECK_EQUAL(aviso.url(), "%ECF_AVISO_URL%");
    BOOST_CHECK_EQUAL(aviso.schema(), "%ECF_AVISO_SCHEMA%");
    BOOST_CHECK_EQUAL(aviso.auth(), "%ECF_AVISO_AUTH%");
    BOOST_CHECK_EQUAL(aviso.polling(), "%ECF_AVISO_POLLING%");
    BOOST_CHECK_EQUAL(aviso.active(), "");
    BOOST_CHECK_EQUAL(aviso.reason(), "''");

    aviso.start();
    // Ensure that the variable substitution is done correctly
    BOOST_CHECK_EQUAL(aviso.active(), R"({ "event": "mars", "request": { "class": "od" } })");

    // This is the first multithreaded test in ecFlow!

    aviso.finish();
    // Ensure that, after finishing, the active listener is cleared
    BOOST_CHECK_EQUAL(aviso.active(), R"()");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
