// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace legacy {

struct AvisoAttrV1
{
    // This is AvisoAttr as serialised by ecFlow 5.19.x, carrying the Aviso v1 fields `schema` and `polling`.

    std::string parent_path;
    std::string name;
    std::string listener;
    std::string url;
    std::string schema;
    std::string polling;
    std::string auth;
    std::string reason;
    std::uint64_t revision = 0;
    std::string active;

    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar & parent_path;
        ar & name;
        ar & listener;
        ar & url;
        ar & schema;
        ar & polling;
        ar & auth;
        ar & reason;
        ar & revision;
        ar & active;
    }
};

} // namespace legacy

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

    const auto& suites = defs.suites();
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
    BOOST_CHECK_EQUAL(aviso.auth(), "%ECF_AVISO_AUTH%");
    BOOST_CHECK_EQUAL(aviso.revision(), 0u);
    BOOST_CHECK_EQUAL(aviso.reason(), "''");
}

BOOST_AUTO_TEST_CASE(can_parse_aviso_attribute_on_task_with_all_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              aviso --name A --listener '{ "event": "mars", "request": { "class": "od"} }' --url http://host:port --auth /path/to/auth --revision 42 --reason 'this is a reason'
          endfamily
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_CHECK_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);

    const auto& suites = defs.suites();
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
    BOOST_CHECK_EQUAL(aviso.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(aviso.revision(), 42u);
    BOOST_CHECK_EQUAL(aviso.reason(), "'this is a reason'");
}

BOOST_AUTO_TEST_CASE(cannot_parse_aviso_attribute_with_aviso_v1_options) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    for (const std::string option : {"--schema /path/to/schema", "--polling 60"}) {
        std::string definition = R"(
            suite s1
              task t1
                aviso --name A --listener '{ "event": "mars", "request": { "class": "od"} }' )" +
                                 option + R"(
            endsuite
        )";

        Defs defs;
        DefsStructureParser parser(&defs, definition, true);

        std::string errorMsg, warningMsg;
        bool parsedOK = parser.doParse(errorMsg, warningMsg);
        BOOST_CHECK_MESSAGE(!parsedOK, "Expected failure to parse Aviso v1 option: " << option);
        BOOST_CHECK_MESSAGE(errorMsg.find("ecFlow 5.19.x is the last release that supports Aviso v1") !=
                                std::string::npos,
                            "Expected the error to name the last release supporting Aviso v1, but got: " << errorMsg);
    }
}

BOOST_AUTO_TEST_CASE(can_parse_checkpoint_with_aviso_v1_options) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    // A definition with state (e.g. a checkpoint written by ecFlow 5.19.x) may carry the Aviso v1 options
    std::string definition = R"(defs_state MIGRATE
suite s1
  task t1
    aviso --name A --listener '{ "event": "mars", "request": { "class": "od"} }' --url http://host:port --schema /path/to/schema --polling 60 --revision 7 --auth /path/to/auth
endsuite
)";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_REQUIRE_MESSAGE(parsedOK, "Failed to parse checkpoint: " << errorMsg);

    const auto& avisos = defs.suites()[0]->taskVec()[0]->avisos();
    BOOST_REQUIRE_EQUAL(avisos.size(), static_cast<size_t>(1));

    const auto& aviso = avisos[0];
    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.url(), "http://host:port");
    BOOST_CHECK_EQUAL(aviso.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(aviso.revision(), 7u);
}

BOOST_AUTO_TEST_CASE(can_deserialise_aviso_written_by_ecflow_5_19) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    legacy::AvisoAttrV1 legacy;
    legacy.parent_path = "/s/f/t";
    legacy.name        = "A";
    legacy.listener    = R"('{ "event": "mars" }')";
    legacy.url         = "http://host:port";
    legacy.schema      = "/path/to/schema";
    legacy.polling     = "60";
    legacy.auth        = "/path/to/auth";
    legacy.reason      = "''";
    legacy.revision    = 42;

    std::string data;
    ecf::save_as_string(data, legacy);

    AvisoAttr restored;
    ecf::restore_from_string(data, restored);

    // All fields, except the Aviso v1 fields, are restored
    BOOST_CHECK_EQUAL(restored.name(), "A");
    BOOST_CHECK_EQUAL(restored.listener(), R"('{ "event": "mars" }')");
    BOOST_CHECK_EQUAL(restored.url(), "http://host:port");
    BOOST_CHECK_EQUAL(restored.auth(), "/path/to/auth");
    BOOST_CHECK_EQUAL(restored.reason(), "''");
    BOOST_CHECK_EQUAL(restored.revision(), 42u);
}

BOOST_AUTO_TEST_CASE(can_serialise_aviso_readable_by_ecflow_5_19) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    AvisoAttr original{nullptr, "A", R"({ "event": "mars" })", "http://host:port", 42, "/path/to/auth", ""};

    std::string data;
    ecf::save_as_string(data, original);

    legacy::AvisoAttrV1 restored;
    ecf::restore_from_string(data, restored);

    // The Aviso v1 fields are kept as empty placeholders, so that all other fields are found in place
    BOOST_CHECK_EQUAL(restored.name, "A");
    BOOST_CHECK_EQUAL(restored.listener, R"('{ "event": "mars" }')");
    BOOST_CHECK_EQUAL(restored.url, "http://host:port");
    BOOST_CHECK_EQUAL(restored.schema, "");
    BOOST_CHECK_EQUAL(restored.polling, "");
    BOOST_CHECK_EQUAL(restored.auth, "/path/to/auth");
    BOOST_CHECK_EQUAL(restored.reason, "''");
    BOOST_CHECK_EQUAL(restored.revision, 42u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
