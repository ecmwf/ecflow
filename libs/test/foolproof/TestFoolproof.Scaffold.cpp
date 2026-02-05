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

#include "scaffold/Naming.hpp"
#include "scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_Scaffold)

BOOST_AUTO_TEST_CASE(test_creating_host) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    {
        auto host = MakeHost{}.create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == Host::resolve_hostname());
    }

    {
        const std::string custom_name = "custom";
        auto host                     = MakeHost{}.with(custom_name).create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == custom_name);
    }

    {
        BOOST_CHECK_EXCEPTION(
            [[maybe_unused]] auto host = MakeHost{}.with(std::string{}).create(),
            std::runtime_error,
            [](const std::runtime_error& e) { return std::string(e.what()) == "host name cannot be empty\n"; });
    }
}

BOOST_AUTO_TEST_CASE(test_locking_known_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace foolproof::scaffold;

    auto port_a = MakePort{}.with(SpecificPortValue{44444}).create();
    auto port_b = MakePort{}.with(SpecificPortValue{44445}).create();
    auto port_c = MakePort{}.with(SpecificPortValue{44446}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() == 44444);
    BOOST_CHECK(port_b.value() == 44445);
    BOOST_CHECK(port_c.value() == 44446);
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_locking_automatic_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace foolproof::scaffold;

    auto port_a = MakePort{}.with(AutomaticPortValue{}).create();
    auto port_b = MakePort{}.with(AutomaticPortValue{44444}).create();
    auto port_c = MakePort{}.with(AutomaticPortValue{44444}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() != port_b.value());
    BOOST_CHECK(port_a.value() != port_c.value());
    BOOST_CHECK(port_b.value() != port_c.value());
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_setting_environment_variables) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    {
        std::string name     = "";
        std::string expected = "value";
        BOOST_CHECK_EXCEPTION([[maybe_unused]] auto variable = MakeEnvironmentVariable{}.with(name, expected).create(),
                              MakeEnvironmentVariable::InvalidVariableName,
                              [](const MakeEnvironmentVariable::InvalidVariableName& e) {
                                  return std::string(e.what()) == "Environment variable name cannot be empty";
                              });
    }

    std::string name = "FOOLPROOF_VARIABLE";
    {
        std::string expected = "";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    auto retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());

    {
        std::string expected = "value";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());
}

BOOST_AUTO_TEST_CASE(test_creating_working_directory) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    BOOST_CHECK(!cwd.path().string().empty());
    BOOST_CHECK(fs::exists(cwd.path()));
    BOOST_CHECK(fs::is_directory(cwd.path()));
    BOOST_CHECK(cwd.path().is_absolute());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
