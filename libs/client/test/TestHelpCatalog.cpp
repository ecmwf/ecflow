/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/client/HelpCatalog.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests ecf::HelpCatalog's lazy access to the embedded CLI help manifest
///

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_HelpCatalog)

BOOST_AUTO_TEST_CASE(test_manifest_parses_and_has_expected_top_level_shape) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json& manifest = ecf::HelpCatalog::manifest();

    BOOST_REQUIRE(manifest.is_object());
    BOOST_CHECK_EQUAL(manifest.at("schema_version").get<int>(), 1);
    BOOST_CHECK(manifest.at("topics").is_array());
    BOOST_CHECK(manifest.at("commands").is_array());
    BOOST_CHECK(manifest.at("options").is_array());
    BOOST_CHECK(manifest.at("environment_variables").is_array());
}

BOOST_AUTO_TEST_CASE(test_manifest_contains_known_entries) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json& manifest = ecf::HelpCatalog::manifest();

    auto has_name = [](const nlohmann::json& array, const std::string& name) {
        for (const auto& entry : array) {
            if (entry.at("name") == name) {
                return true;
            }
        }
        return false;
    };

    BOOST_CHECK(has_name(manifest.at("commands"), "abort"));
    BOOST_CHECK(has_name(manifest.at("options"), "host"));
    BOOST_CHECK(has_name(manifest.at("environment_variables"), "ECF_HOST"));
    BOOST_CHECK(has_name(manifest.at("topics"), "summary"));
}

BOOST_AUTO_TEST_CASE(test_manifest_is_parsed_once_and_cached) {
    ECF_NAME_THIS_TEST();

    // Same reference on every call confirms the manifest is parsed lazily,
    // once, and cached -- not re-parsed on every invocation.
    const nlohmann::json& first  = ecf::HelpCatalog::manifest();
    const nlohmann::json& second = ecf::HelpCatalog::manifest();
    BOOST_CHECK_EQUAL(&first, &second);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
