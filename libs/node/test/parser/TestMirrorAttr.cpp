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

#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_MirrorAttr)

BOOST_AUTO_TEST_CASE(can_parse_mirror_attribute_on_task_with_default_parameters) {
    ECF_NAME_THIS_TEST();

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

    const auto& suites = defs.suites();
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
    BOOST_CHECK_EQUAL(mirror.propagate(), false);
}

BOOST_AUTO_TEST_CASE(can_parse_mirror_attribute_on_task_with_all_attributes) {
    ECF_NAME_THIS_TEST();

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

    const auto& suites = defs.suites();
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
    BOOST_CHECK_EQUAL(mirror.propagate(), false);
}

BOOST_AUTO_TEST_CASE(can_parse_mirror_attribute_on_task_with_ssl_and_propagate) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              mirror --name A --remote_path /s1/f1/t2 --remote_host hostname --remote_port 1234 --polling 20 --ssl --propagate
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

    const auto& mirrors = tasks[0]->mirrors();
    BOOST_CHECK_EQUAL(mirrors.size(), static_cast<size_t>(1));

    const auto& mirror = mirrors[0];
    BOOST_CHECK_EQUAL(mirror.name(), "A");
    BOOST_CHECK_EQUAL(mirror.remote_path(), "/s1/f1/t2");
    BOOST_CHECK_EQUAL(mirror.remote_host(), "hostname");
    BOOST_CHECK_EQUAL(mirror.remote_port(), "1234");
    BOOST_CHECK_EQUAL(mirror.polling(), "20");
    BOOST_CHECK_EQUAL(mirror.ssl(), true);
    BOOST_CHECK_EQUAL(mirror.propagate(), true);
}

BOOST_AUTO_TEST_CASE(can_roundtrip_serialise_mirror_preserving_propagate_option) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    {
        MirrorAttr original{nullptr, "A", "/s/f/t", "host", "1234", "20", true, "auth", "", true};
        std::string data;
        ecf::save_as_string(data, original);

        MirrorAttr restored;
        ecf::restore_from_string(data, restored);

        BOOST_CHECK_EQUAL(restored.name(), "A");
        BOOST_CHECK_EQUAL(restored.remote_path(), "/s/f/t");
        BOOST_CHECK_EQUAL(restored.remote_host(), "host");
        BOOST_CHECK_EQUAL(restored.remote_port(), "1234");
        BOOST_CHECK_EQUAL(restored.polling(), "20");
        BOOST_CHECK_EQUAL(restored.ssl(), true);
        BOOST_CHECK_EQUAL(restored.auth(), "auth");
        BOOST_CHECK_EQUAL(restored.reason(), "");
        BOOST_CHECK_EQUAL(restored.propagate(), true);
    }

    {
        MirrorAttr original{nullptr, "B", "/s/f/t", "host", "1234", "20", false, "auth", "", false};
        std::string data;
        ecf::save_as_string(data, original);

        MirrorAttr restored;
        ecf::restore_from_string(data, restored);

        BOOST_CHECK_EQUAL(restored.name(), "B");
        BOOST_CHECK_EQUAL(restored.remote_path(), "/s/f/t");
        BOOST_CHECK_EQUAL(restored.remote_host(), "host");
        BOOST_CHECK_EQUAL(restored.remote_port(), "1234");
        BOOST_CHECK_EQUAL(restored.polling(), "20");
        BOOST_CHECK_EQUAL(restored.ssl(), false);
        BOOST_CHECK_EQUAL(restored.auth(), "auth");
        BOOST_CHECK_EQUAL(restored.reason(), "");
        BOOST_CHECK_EQUAL(restored.propagate(), false);
    }
}

namespace legacy {

struct MirrorAttrV0
{
    // This is MirrorAttr V0, which does *not* carry the `propagate_` field.
    // The absence of CEREAL_CLASS_VERSION means the archive is stamped with version 0.

    std::string name;
    std::string remote_path;
    std::string remote_host;
    std::string remote_port;
    std::string polling;
    bool ssl = false;
    std::string auth;
    std::string reason;

    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/) {
        ar & name;
        ar & remote_path;
        ar & remote_host;
        ar & remote_port;
        ar & polling;
        ar & ssl;
        ar & auth;
        ar & reason;
    }
};

} // namespace legacy

BOOST_AUTO_TEST_CASE(can_deserialise_mirror_v0_with_propagate_defaulting_to_false) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    legacy::MirrorAttrV0 legacy;
    legacy.name        = "A";
    legacy.remote_path = "/s/f/t";
    legacy.remote_host = "host";
    legacy.remote_port = "1234";
    legacy.polling     = "20";
    legacy.ssl         = true;
    legacy.auth        = "auth";
    legacy.reason      = "";

    std::string data;
    ecf::save_as_string(data, legacy);

    MirrorAttr restored;
    ecf::restore_from_string(data, restored);

    // Fields present in v0 are restored as expected...
    BOOST_CHECK_EQUAL(restored.name(), "A");
    BOOST_CHECK_EQUAL(restored.remote_path(), "/s/f/t");
    BOOST_CHECK_EQUAL(restored.remote_host(), "host");
    BOOST_CHECK_EQUAL(restored.remote_port(), "1234");
    BOOST_CHECK_EQUAL(restored.polling(), "20");
    BOOST_CHECK_EQUAL(restored.ssl(), true);
    BOOST_CHECK_EQUAL(restored.auth(), "auth");
    BOOST_CHECK_EQUAL(restored.reason(), "");
    // ...and the absent `propagate` field defaults deterministically to false (rather than an indeterminate value)
    BOOST_CHECK_EQUAL(restored.propagate(), false);
}

BOOST_AUTO_TEST_CASE(can_roundtrip_print_parse_mirror_preserving_propagate_option) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    std::string definition = R"(
        suite s1
          family f1
            task t1
              mirror --name A --remote_path /s1/f1/t2 --remote_host hostname --remote_port 1234 --polling 20 --ssl --propagate
          endfamily
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(parser.doParse(errorMsg, warningMsg), "Failed to parse definition: " << errorMsg);

    // Print the in-memory defs back to the text definition format...
    std::string printed = ecf::as_string(defs, PrintStyle::DEFS);
    BOOST_CHECK_MESSAGE(printed.find("--propagate") != std::string::npos,
                        "Expected '--propagate' in printed defs:\n"
                            << printed);

    // ...and re-parse it, ensuring the `propagate` flag survives the print/parse round-trip
    Defs reparsed;
    DefsStructureParser reparser(&reparsed, printed, true);
    BOOST_REQUIRE_MESSAGE(reparser.doParse(errorMsg, warningMsg),
                          "Failed to re-parse printed definition: " << errorMsg << "\n"
                                                                    << printed);

    const auto& mirror = reparsed.suites()[0]->familyVec()[0]->taskVec()[0]->mirrors()[0];
    BOOST_CHECK_EQUAL(mirror.ssl(), true);
    BOOST_CHECK_EQUAL(mirror.propagate(), true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
