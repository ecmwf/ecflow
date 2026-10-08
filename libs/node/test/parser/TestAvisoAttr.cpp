// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
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
        // The literal text (not the constant) guards the user-facing pointer to the last release supporting Aviso v1
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

BOOST_AUTO_TEST_CASE(can_parse_and_print_collapse_option) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          task t1
            aviso --name A --listener '{ "event": "mars" }' --collapse
          task t2
            aviso --name B --listener '{ "event": "mars" }'
        endsuite
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(parser.doParse(errorMsg, warningMsg), "Failed to parse definition: " << errorMsg);

    BOOST_CHECK(defs.suites()[0]->taskVec()[0]->avisos()[0].collapse());
    BOOST_CHECK(!defs.suites()[0]->taskVec()[1]->avisos()[0].collapse());

    // The option is printed only when set, and survives a print/parse round trip
    std::string printed = ecf::as_string(defs, PrintStyle::DEFS);
    auto first          = printed.find("--collapse");
    BOOST_REQUIRE(first != std::string::npos);
    BOOST_CHECK(printed.find("--collapse", first + 1) == std::string::npos);

    Defs reparsed;
    DefsStructureParser reparser(&reparsed, printed, true);
    BOOST_REQUIRE_MESSAGE(reparser.doParse(errorMsg, warningMsg), "Failed to re-parse: " << errorMsg);
    BOOST_CHECK(reparsed.suites()[0]->taskVec()[0]->avisos()[0].collapse());
    BOOST_CHECK(!reparsed.suites()[0]->taskVec()[1]->avisos()[0].collapse());
}

BOOST_AUTO_TEST_CASE(can_serialise_collapse_option) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    AvisoAttr original{nullptr, "A", R"({ "event": "mars" })", "http://host:port", 42, "/path/to/auth", "", true};

    std::string data;
    ecf::save_as_string(data, original);

    AvisoAttr restored;
    ecf::restore_from_string(data, restored);

    BOOST_CHECK(restored.collapse());
    BOOST_CHECK(restored == original);
}

BOOST_AUTO_TEST_CASE(deserialises_aviso_written_by_ecflow_5_19_without_collapse) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    legacy::AvisoAttrV1 legacy;
    legacy.name     = "A";
    legacy.listener = R"('{ "event": "mars" }')";
    legacy.reason   = "''";

    std::string data;
    ecf::save_as_string(data, legacy);

    AvisoAttr restored;
    ecf::restore_from_string(data, restored);

    // The archive of ecFlow 5.19.x has no class version (i.e. version 0), no collapse option and no event
    BOOST_CHECK(!restored.collapse());
    BOOST_CHECK(restored.event().empty());
}

BOOST_AUTO_TEST_CASE(writes_event_in_checkpoints_only) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          task t1
            aviso --name A --listener '{ "event": "mars" }'
        endsuite
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(parser.doParse(errorMsg, warningMsg), "Failed to parse definition: " << errorMsg);

    // A payload holding single quotes and percent signs, which must survive the quoting of the option
    const std::string payload = R"({"location":"file:///it's/100%/x"})";
    auto& aviso               = defs.suites()[0]->taskVec()[0]->avisos()[0];
    const AvisoEvent event{"mars", 7, R"({"class":"od"})", payload};
    aviso.set_event(event);

    BOOST_CHECK(ecf::as_string(defs, PrintStyle::DEFS).find("--event") == std::string::npos);

    std::string checkpoint = ecf::as_string(defs, PrintStyle::MIGRATE);
    BOOST_REQUIRE(checkpoint.find("--event") != std::string::npos);

    Defs restored;
    DefsStructureParser reparser(&restored, checkpoint, true);
    BOOST_REQUIRE_MESSAGE(reparser.doParse(errorMsg, warningMsg), "Failed to parse checkpoint: " << errorMsg);

    const auto& restored_aviso = restored.suites()[0]->taskVec()[0]->avisos()[0];
    BOOST_CHECK(restored_aviso.event() == event);
}

BOOST_AUTO_TEST_CASE(writes_free_in_checkpoints_only) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          task t1
            aviso --name A --listener '{ "event": "mars" }'
        endsuite
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(parser.doParse(errorMsg, warningMsg), "Failed to parse definition: " << errorMsg);

    auto& aviso = defs.suites()[0]->taskVec()[0]->avisos()[0];
    BOOST_REQUIRE(!aviso.isSetFree());
    aviso.setFree();

    BOOST_CHECK(ecf::as_string(defs, PrintStyle::DEFS).find("--free") == std::string::npos);

    std::string checkpoint = ecf::as_string(defs, PrintStyle::MIGRATE);
    BOOST_REQUIRE(checkpoint.find("--free") != std::string::npos);

    Defs restored;
    DefsStructureParser reparser(&restored, checkpoint, true);
    BOOST_REQUIRE_MESSAGE(reparser.doParse(errorMsg, warningMsg), "Failed to parse checkpoint: " << errorMsg);
    BOOST_CHECK(restored.suites()[0]->taskVec()[0]->avisos()[0].isSetFree());
}

BOOST_AUTO_TEST_CASE(can_serialise_free) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    AvisoAttr original{nullptr, "A", R"({ "event": "mars" })", "http://host:port", 7, "/path/to/auth", ""};
    original.setFree();

    std::string data;
    ecf::save_as_string(data, original);

    AvisoAttr restored;
    ecf::restore_from_string(data, restored);

    BOOST_CHECK(restored == original);
    BOOST_CHECK(restored.isSetFree());
}

BOOST_AUTO_TEST_CASE(event_option_round_trips) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    const AvisoEvent event{"mars", 7, R"({"class":"od"})", R"({"location":"file:///it's/x"})"};

    const auto option = event.to_option();
    BOOST_CHECK_EQUAL(option.front(), '\'');
    BOOST_CHECK_EQUAL(option.back(), '\'');
    BOOST_CHECK(option.find('\'', 1) == option.size() - 1);

    // The value is accepted with and without the surrounding single quotes
    BOOST_CHECK(AvisoEvent::from_option(option) == event);
    BOOST_CHECK(AvisoEvent::from_option(option.substr(1, option.size() - 2)) == event);
}

BOOST_AUTO_TEST_CASE(event_describes_notification) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    const ecf::service::aviso::AvisoNotification notification{"mars", 7, R"({"class":"od"})", R"({"x":1})"};
    const auto event = AvisoEvent::from(notification);

    BOOST_CHECK_EQUAL(event.type, "mars");
    BOOST_CHECK_EQUAL(event.sequence, 7u);
    BOOST_CHECK_EQUAL(event.identifier, R"({"class":"od"})");
    BOOST_CHECK_EQUAL(event.payload, R"({"x":1})");
    BOOST_CHECK(!event.empty());
    BOOST_CHECK(AvisoEvent{}.empty());
}

BOOST_AUTO_TEST_CASE(cannot_parse_invalid_event) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    for (const std::string event : {"'not json'", R"('{ "type": "mars" }')", R"('{ "type": 1, "sequence": 1 }')"}) {
        std::string definition = "defs_state MIGRATE\nsuite s1\n  task t1\n    aviso --name A --listener '{ \"event\": "
                                 "\"mars\" }' --event " +
                                 event + "\nendsuite\n";

        Defs defs;
        DefsStructureParser parser(&defs, definition, true);
        std::string errorMsg, warningMsg;
        BOOST_CHECK_MESSAGE(!parser.doParse(errorMsg, warningMsg), "Expected invalid event to be rejected: " << event);
    }
}

BOOST_AUTO_TEST_CASE(can_serialise_event) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    AvisoAttr original{nullptr, "A", R"({ "event": "mars" })", "http://host:port", 7, "/path/to/auth", ""};
    original.set_event(AvisoEvent{"mars", 7, R"({"class":"od"})", R"({"location":"file:///x"})"});

    std::string data;
    ecf::save_as_string(data, original);

    AvisoAttr restored;
    ecf::restore_from_string(data, restored);

    BOOST_CHECK(restored == original);
    BOOST_CHECK_EQUAL(restored.event().payload, R"({"location":"file:///x"})");
}

BOOST_AUTO_TEST_CASE(cannot_parse_aviso_on_family_or_suite) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    const std::string aviso = "aviso --name A --listener '{ \"event\": \"mars\" }'";

    // Rejected in definitions, and in definitions with state (e.g. check points)
    for (const auto& header : {std::string{}, std::string{"defs_state MIGRATE\n"}}) {
        for (const auto& body : {"suite s\n  " + aviso + "\n  task t\nendsuite\n",
                                 "suite s\n  family f\n    " + aviso + "\n    task t\n  endfamily\nendsuite\n"}) {
            Defs defs;
            DefsStructureParser parser(&defs, header + body, true);
            std::string errorMsg, warningMsg;
            BOOST_CHECK_MESSAGE(!parser.doParse(errorMsg, warningMsg), "Expected rejection of:\n" << header + body);
            BOOST_CHECK_MESSAGE(errorMsg.find("only allowed on a task") != std::string::npos, errorMsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(cannot_parse_aviso_and_mirror_on_the_same_node) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    const std::string aviso  = "aviso --name A --listener '{ \"event\": \"mars\" }'";
    const std::string mirror = "mirror --name M --remote_path /s/t";

    // Rejected in either order, in definitions, and in definitions with state (e.g. check points)
    for (const auto& header : {std::string{}, std::string{"defs_state MIGRATE\n"}}) {
        for (const auto& body : {"suite s\n  task t\n    " + aviso + "\n    " + mirror + "\nendsuite\n",
                                 "suite s\n  task t\n    " + mirror + "\n    " + aviso + "\nendsuite\n"}) {
            Defs defs;
            DefsStructureParser parser(&defs, header + body, true);
            std::string errorMsg, warningMsg;
            BOOST_CHECK_MESSAGE(!parser.doParse(errorMsg, warningMsg), "Expected rejection of:\n" << header + body);
            BOOST_CHECK_MESSAGE(errorMsg.find("not allowed on a node with a") != std::string::npos, errorMsg);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
