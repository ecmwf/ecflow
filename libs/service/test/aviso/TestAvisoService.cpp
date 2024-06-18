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

#include "TestContentProvider.hpp"
#include "ecflow/service/Registry.hpp"
#include "ecflow/service/aviso/AvisoService.hpp"
#include "ecflow/service/aviso/etcd/Client.hpp"

BOOST_AUTO_TEST_SUITE(U_AvisoService)

BOOST_AUTO_TEST_SUITE(T_Etcd_Client)

BOOST_AUTO_TEST_CASE(can_create_etcd_client) {
    using namespace ecf::service::aviso;
    using namespace ecf::service::aviso::etcd;

    std::string address = "http://address:1234";
    Client client{address};

    BOOST_CHECK_EQUAL(client.address(), address);
    BOOST_CHECK_EQUAL(client.auth_token(), "");
}

BOOST_AUTO_TEST_CASE(can_poll_using_etcd_client) {
    using namespace ecf::service::aviso;
    using namespace ecf::service::aviso::etcd;

    std::string address = "http://address:1234";
    Client client{address};
    BOOST_CHECK_EXCEPTION(client.poll("/prefix", 1), std::runtime_error, [](const std::runtime_error& e) {
        return std::string{e.what()}.find("EtcdClient: Unable to retrieve result, due to ") != std::string::npos;
    });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_NotificationPackage)

BOOST_AUTO_TEST_CASE(can_print_aviso_notification_package) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
             {
                 "event": "dissemination",
                 "request": {
                     "destination": "COM",
                     "date": 20000101,
                     "class": "od",
                     "expver": ["0001", "0002"],
                     "step": [0, 12]
                 }
             }
         )";
    std::string address        = "http://server:1234";
    std::string schema_content = R"(
             {
                 "dissemination": {
                     "endpoint": [
                         {
                             "engine":[
                                 "etcd_rest"
                             ],
                             "base":"/ec/diss/{destination}",
                             "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                             "admin":"/ec/admin/{date}/{destination}"
                         }
                     ]
                 }
             }
         )";
    uint32_t polling           = 60;
    uint64_t revision          = 1;

    ConfiguredListener listener =
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision);

    AvisoNotification notification{"key", "value", 42};

    NotificationPackage<ConfiguredListener, AvisoNotification> package{"path", listener, notification};

    std::ostringstream oss;
    oss << package;
    BOOST_CHECK(oss.str().find("NotificationPackage") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoResponse)

BOOST_AUTO_TEST_CASE(can_print_aviso_response_with_notification_package) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
             {
                 "event": "dissemination",
                 "request": {
                     "destination": "COM",
                     "date": 20000101,
                     "class": "od",
                     "expver": ["0001", "0002"],
                     "step": [0, 12]
                 }
             }
         )";
    std::string address        = "http://server:1234";
    std::string schema_content = R"(
             {
                 "dissemination": {
                     "endpoint": [
                         {
                             "engine":[
                                 "etcd_rest"
                             ],
                             "base":"/ec/diss/{destination}",
                             "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                             "admin":"/ec/admin/{date}/{destination}"
                         }
                     ]
                 }
             }
         )";
    uint32_t polling           = 60;
    uint64_t revision          = 1;

    ConfiguredListener listener =
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision);

    AvisoNotification notification{"key", "value", 42};

    NotificationPackage<ConfiguredListener, AvisoNotification> package{"path", listener, notification};

    AvisoResponse response = package;

    std::ostringstream oss;
    oss << response;
    BOOST_CHECK(oss.str().find("NotificationPackage") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_print_aviso_response_with_no_match) {
    using namespace ecf::service::aviso;

    AvisoNoMatch no_match;
    AvisoResponse response = no_match;

    std::ostringstream oss;
    oss << response;
    BOOST_CHECK(oss.str().find("AvisoNoMatch") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_print_aviso_response_with_error) {
    using namespace ecf::service::aviso;

    AvisoError error{"error"};
    AvisoResponse response = error;

    std::ostringstream oss;
    oss << response;
    BOOST_CHECK(oss.str().find("AvisoError") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoService_Entry)

BOOST_AUTO_TEST_CASE(can_create_parameterised_aviso_service_entry) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
             {
                 "event": "dissemination",
                 "request": {
                     "destination": "COM",
                     "date": 20000101,
                     "class": "od",
                     "expver": ["0001", "0002"],
                     "step": [0, 12]
                 }
             }
         )";
    std::string address        = "http://server:1234";
    std::string schema_content = R"(
             {
                 "dissemination": {
                     "endpoint": [
                         {
                             "engine":[
                                 "etcd_rest"
                             ],
                             "base":"/ec/diss/{destination}",
                             "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                             "admin":"/ec/admin/{date}/{destination}"
                         }
                     ]
                 }
             }
         )";
    uint32_t polling           = 60;
    uint64_t revision          = 1;

    ConfiguredListener listener =
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision);
    {
        AvisoService::Entry entry{listener};

        BOOST_CHECK(entry.listener().address() == listener.address());
        BOOST_CHECK(entry.listener().prefix() == listener.prefix());
        BOOST_CHECK(entry.listener().path() == listener.path());
    }
    {
        const AvisoService::Entry entry{listener};

        BOOST_CHECK(entry.listener().address() == listener.address());
        BOOST_CHECK(entry.listener().prefix() == listener.prefix());
        BOOST_CHECK(entry.listener().path() == listener.path());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoController)

BOOST_AUTO_TEST_CASE(can_create_start_and_stop_aviso_controller) {
    using namespace ecf::service::aviso;

    std::string listener_cfg   = R"(
             {
                 "event": "dissemination",
                 "request": {
                     "destination": "COM"
                 }
             }
         )";
    std::string schema_content = R"(
             {
                 "dissemination": {
                     "endpoint": [
                         {
                             "engine":[
                                 "etcd_rest"
                             ],
                             "base":"/ec/diss/{destination}",
                             "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                             "admin":"/ec/admin/{date}/{destination}"
                         }
                     ]
                 }
             }
         )";
    std::string auth_content   = R"(
             {
                 "url": "http://address:1234",
                 "key": "00000000111111110000000011111111",
                 "email": "user@host.int"
             }
         )";

    ecf::test::TestContentProvider schema_content_provider{"aviso_schema_test", schema_content};
    ecf::test::TestContentProvider auth_content_provider{"aviso_auth_test", auth_content};

    ecf::service::TheOneServer::set_server(nullptr);

    AvisoController controller;

    // Subscribe to a listener for /s1/f1/t1
    controller.subscribe(AvisoRequest{AvisoSubscribe{
        "/s1/f1/t1", listener_cfg, "address", schema_content_provider.file(), 60, 1, auth_content_provider.file()}});

    // Subscribe and unsubcribe to a listener for /s1/f1/t2
    controller.subscribe(AvisoRequest{AvisoSubscribe{
        "/s1/f1/t2", listener_cfg, "address", schema_content_provider.file(), 60, 1, auth_content_provider.file()}});
    controller.subscribe(AvisoRequest{AvisoUnsubscribe{"/s1/f1/t2"}});

    controller.start();

    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    controller.stop();
    controller.terminate();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
