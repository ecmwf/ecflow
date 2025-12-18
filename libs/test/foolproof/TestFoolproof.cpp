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

BOOST_AUTO_TEST_SUITE(T_basic)

BOOST_AUTO_TEST_CASE(test_locking_known_ports) {

    using namespace ecf;

    WithPort port_a{SpecificPortValue{44444}};
    WithPort port_b{SpecificPortValue{44445}};
    WithPort port_c{SpecificPortValue{44446}};

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

    WithPort port_a{AutomaticPortValue{}};
    WithPort port_b{AutomaticPortValue{44444}};
    WithPort port_c{AutomaticPortValue{44444}};

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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(test_end_to_end_functionality) {

    WithPort port{AutomaticPortValue{}};

    WithServer server{port.value()};
}

BOOST_AUTO_TEST_SUITE_END()
