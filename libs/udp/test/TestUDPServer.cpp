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

#include "TestSupport.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace ut = boost::unit_test;

BOOST_FIXTURE_TEST_SUITE(S_UDP, ecf::test::EnableServersFixture)

BOOST_AUTO_TEST_SUITE(T_UDPServer)

BOOST_AUTO_TEST_CASE(can_update_events) {
    ECF_NAME_THIS_TEST();

    // Family f2
    {
        auto event = ecflow_server.get_event("/s1/f2", "event_at_f2");
        BOOST_TEST(!event.value());

        ecflow_udp.set_event("/s1/f2", "event_at_f2");
        event = ecflow_server.get_event("/s1/f2", "event_at_f2");
        BOOST_TEST(event.value());

        ecflow_udp.clear_event("/s1/f2", "event_at_f2");
        event = ecflow_server.get_event("/s1/f2", "event_at_f2");
        BOOST_TEST(!event.value());
    }
    // Family f3
    {
        auto event = ecflow_server.get_event("/s1/f2/f3", "event_at_f3");
        BOOST_TEST(!event.value());

        ecflow_udp.set_event("/s1/f2/f3", "event_at_f3");
        event = ecflow_server.get_event("/s1/f2/f3", "event_at_f3");
        BOOST_TEST(event.value());

        ecflow_udp.clear_event("/s1/f2/f3", "event_at_f3");
        event = ecflow_server.get_event("/s1/f2/f3", "event_at_f3");
        BOOST_TEST(!event.value());
    }
    // Task t4
    {
        auto event = ecflow_server.get_event("/s1/f2/f3/t4", "event_at_t4");
        BOOST_TEST(!event.value());

        ecflow_udp.set_event("/s1/f2/f3/t4", "event_at_t4");
        event = ecflow_server.get_event("/s1/f2/f3/t4", "event_at_t4");
        BOOST_TEST(event.value());

        ecflow_udp.clear_event("/s1/f2/f3/t4", "event_at_t4");
        event = ecflow_server.get_event("/s1/f2/f3/t4", "event_at_t4");
        BOOST_TEST(!event.value());
    }
}

BOOST_AUTO_TEST_CASE(can_update_labels) {
    ECF_NAME_THIS_TEST();

    // Family f2
    {
        auto label = ecflow_server.get_label("/s1/f2", "label_at_f2");
        BOOST_TEST(label.new_value() == "");
    }
    {
        ecflow_udp.update_label("/s1/f2", "label_at_f2", "some_value");
        auto label = ecflow_server.get_label("/s1/f2", "label_at_f2");
        BOOST_TEST(label.new_value() == "some_value");
    }
    // Family f3
    {
        auto label = ecflow_server.get_label("/s1/f2/f3", "label_at_f3");
        BOOST_TEST(label.new_value() == "");
    }
    {
        ecflow_udp.update_label("/s1/f2/f3", "label_at_f3", "another_value");
        auto label = ecflow_server.get_label("/s1/f2/f3", "label_at_f3");
        BOOST_TEST(label.new_value() == "another_value");
    }
    // Task t4
    {
        auto label = ecflow_server.get_label("/s1/f2/f3/t4", "label_at_t4");
        BOOST_TEST(label.new_value() == "");
    }
    std::string values[] = {"a", "b", "c", "d"};
    for (auto value : values) {
        ecflow_udp.update_label("/s1/f2/f3/t4", "label_at_t4", value);
        auto label = ecflow_server.get_label("/s1/f2/f3/t4", "label_at_t4");
        BOOST_TEST(label.new_value() == value);
    }
}

BOOST_AUTO_TEST_CASE(can_update_meters) {
    ECF_NAME_THIS_TEST();

    // Family f2
    {
        auto meter = ecflow_server.get_meter("/s1/f2", "meter_at_f2");
        BOOST_TEST(meter.value() == 0);
    }
    {
        ecflow_udp.update_meter("/s1/f2", "meter_at_f2", 49);
        auto meter = ecflow_server.get_meter("/s1/f2", "meter_at_f2");
        BOOST_TEST(meter.value() == 49);
    }
    // Family f3
    {
        auto meter = ecflow_server.get_meter("/s1/f2/f3", "meter_at_f3");
        BOOST_TEST(meter.value() == 0);
    }
    {
        ecflow_udp.update_meter("/s1/f2/f3", "meter_at_f3", 50);
        auto meter = ecflow_server.get_meter("/s1/f2/f3", "meter_at_f3");
        BOOST_TEST(meter.value() == 50);
    }
    // Task t4
    {
        auto meter = ecflow_server.get_meter("/s1/f2/f3/t4", "meter_at_t4");
        BOOST_TEST(meter.value() == 0);
    }
    int values[] = {0, 10, 25, 50, 75, 100};
    for (auto value : values) {
        ecflow_udp.update_meter("/s1/f2/f3/t4", "meter_at_t4", value);
        auto meter = ecflow_server.get_meter("/s1/f2/f3/t4", "meter_at_t4");
        BOOST_TEST(meter.value() == value);
    }
}

BOOST_AUTO_TEST_CASE(can_update_meter_outside_valid_range) {
    ECF_NAME_THIS_TEST();

    {
        ecflow_udp.update_meter("/s1/f2/f3/t4", "meter_at_t4", 50);
        auto meter = ecflow_server.get_meter("/s1/f2/f3/t4", "meter_at_t4");
        BOOST_TEST(meter.value() == 50);
    }
    {
        ecflow_udp.update_meter("/s1/f2/f3/t4", "meter_at_t4", -1);
        auto meter = ecflow_server.get_meter("/s1/f2/f3/t4", "meter_at_t4");
        BOOST_TEST(meter.value() == 50);
    }
    {
        ecflow_udp.update_meter("/s1/f2/f3/t4", "meter_at_t4", 101);
        auto meter = ecflow_server.get_meter("/s1/f2/f3/t4", "meter_at_t4");
        BOOST_TEST(meter.value() == 50);
    }
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_json_request) {
    ECF_NAME_THIS_TEST();

    ecflow_udp.send(R"()");
    ecflow_udp.send(R"({})");
    ecflow_udp.send(R"({"method": "invalid", "data": {}})");
    ecflow_udp.send(R"({"method": "put", "data": {}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "invalid"}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "label"}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "label", "name": "a_name"}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "label", "name": "a_name", "path": "a_path}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "meter"}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "meter", "name": "a_name"}})");
    ecflow_udp.send(R"({"method": "put", "data": {"type": "meter", "name": "a_name", "path": "a_path}})");
}

BOOST_AUTO_TEST_CASE(can_authenticate_requests) {
    ECF_NAME_THIS_TEST();

    ecflow_udp.send(R"()");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
