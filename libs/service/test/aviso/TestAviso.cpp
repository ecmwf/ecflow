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

#include "ecflow/service/aviso/Aviso.hpp"

BOOST_AUTO_TEST_SUITE(U_Aviso)

BOOST_AUTO_TEST_SUITE(T_AvisoSubscribe)

BOOST_AUTO_TEST_CASE(can_create_default_aviso_subscribe) {
    using namespace ecf::service::aviso;
    AvisoSubscribe subscribe{"path", "listener_cfg", "address", "schema", 60, 1, "auth"};
    BOOST_CHECK_EQUAL(subscribe.path(), "path");
    BOOST_CHECK_EQUAL(subscribe.listener_cfg(), "listener_cfg");
    BOOST_CHECK_EQUAL(subscribe.address(), "address");
    BOOST_CHECK_EQUAL(subscribe.schema(), "schema");
    BOOST_CHECK_EQUAL(subscribe.polling(), static_cast<uint32_t>(60));
    BOOST_CHECK_EQUAL(subscribe.revision(), static_cast<uint64_t>(1));
    BOOST_CHECK_EQUAL(subscribe.auth(), "auth");
}

BOOST_AUTO_TEST_CASE(can_print_aviso_subscribe) {
    using namespace ecf::service::aviso;
    AvisoSubscribe subscribe{"path", "listener_cfg", "address", "schema", 60, 1, "auth"};

    std::ostringstream oss;
    oss << subscribe;
    BOOST_CHECK(oss.str().find("AvisoSubscribe") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoUnsubscribe)

BOOST_AUTO_TEST_CASE(can_create_default_aviso_unsubscribe) {
    using namespace ecf::service::aviso;
    AvisoUnsubscribe unsubscribe{"path"};
    BOOST_CHECK_EQUAL(unsubscribe.path(), "path");
}

BOOST_AUTO_TEST_CASE(can_print_aviso_unsubscribe) {
    using namespace ecf::service::aviso;
    AvisoUnsubscribe unsubscribe{"path"};

    std::ostringstream oss;
    oss << unsubscribe;
    BOOST_CHECK(oss.str().find("AvisoUnsubscribe") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoRequest)

BOOST_AUTO_TEST_CASE(can_create_aviso_request_to_subscribe) {
    using namespace ecf::service::aviso;
    AvisoRequest request{AvisoSubscribe{"path", "listener_cfg", "address", "schema", 60, 1, "auth"}};
    std::visit([](auto&& arg) { BOOST_CHECK_EQUAL(arg.path(), "path"); }, request);
}

BOOST_AUTO_TEST_CASE(can_create_aviso_request_to_unsubscribe) {
    using namespace ecf::service::aviso;
    AvisoRequest request{AvisoUnsubscribe{"path"}};
    std::visit([](auto&& arg) { BOOST_CHECK_EQUAL(arg.path(), "path"); }, request);
}

BOOST_AUTO_TEST_CASE(can_print_aviso_request) {
    using namespace ecf::service::aviso;
    AvisoRequest request{AvisoSubscribe{"path", "listener_cfg", "address", "schema", 60, 1, "auth"}};

    std::ostringstream oss;
    oss << request;
    BOOST_CHECK(oss.str().find("AvisoRequest") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoNotification)

BOOST_AUTO_TEST_CASE(can_create_parameterised_aviso_notification) {
    using namespace ecf::service::aviso;
    AvisoNotification notification{"key", "value", 1};
    notification.add_parameter("parameter1", "parameter_value1");
    notification.add_parameter("parameter2", "parameter_value2");

    BOOST_CHECK_EQUAL(notification.key(), "key");
    BOOST_CHECK_EQUAL(notification.value(), "value");
    BOOST_CHECK_EQUAL(notification.revision(), static_cast<uint64_t>(1));
}

BOOST_AUTO_TEST_CASE(can_print_aviso_notification) {
    using namespace ecf::service::aviso;
    AvisoNotification notification{"key", "value", 1};

    std::ostringstream oss;
    oss << notification;
    BOOST_CHECK(oss.str().find("AvisoNotification") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoError)

BOOST_AUTO_TEST_CASE(can_create_parameterised_aviso_no_match) {
    using namespace ecf::service::aviso;
    [[maybe_unused]] AvisoNoMatch no_match;
}

BOOST_AUTO_TEST_CASE(can_print_aviso_no_match) {
    using namespace ecf::service::aviso;
    AvisoNoMatch no_match;

    std::ostringstream oss;
    oss << no_match;
    BOOST_CHECK(oss.str().find("AvisoNoMatch") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_create_parameterised_aviso_error) {
    using namespace ecf::service::aviso;
    AvisoError error{"error"};
    BOOST_CHECK_EQUAL(error.reason(), "error");
}

BOOST_AUTO_TEST_CASE(can_print_aviso_error) {
    using namespace ecf::service::aviso;
    AvisoError error{"error"};

    std::ostringstream oss;
    oss << error;
    BOOST_CHECK(oss.str().find("AvisoError") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoListener)

BOOST_AUTO_TEST_CASE(can_create_default_listener) {
    using namespace ecf::service::aviso;
    Listener listener;
    BOOST_CHECK_EQUAL(listener.name(), "");
    BOOST_CHECK_EQUAL(listener.base(), "");
    BOOST_CHECK_EQUAL(listener.stem(), "");
    BOOST_CHECK_EQUAL(listener.full(), "/");
}

BOOST_AUTO_TEST_CASE(can_create_parameterised_listener) {
    using namespace ecf::service::aviso;
    Listener listener("name", "/base", "stem");
    BOOST_CHECK_EQUAL(listener.name(), "name");
    BOOST_CHECK_EQUAL(listener.base(), "/base");
    BOOST_CHECK_EQUAL(listener.stem(), "stem");
    BOOST_CHECK_EQUAL(listener.full(), "/base/stem");
}

BOOST_AUTO_TEST_CASE(can_create_parameterised_configured_listener) {
    using namespace ecf::service::aviso;
    ConfiguredListener listener("http://unknown:1234", "path", "name", "/base", "stem", 2, 0);
    BOOST_CHECK_EQUAL(listener.address(), "http://unknown:1234");
    BOOST_CHECK_EQUAL(listener.path(), "path");
    BOOST_CHECK_EQUAL(listener.name(), "name");
    BOOST_CHECK_EQUAL(listener.base(), "/base");
    BOOST_CHECK_EQUAL(listener.resolved_base(), "/base");
    BOOST_CHECK_EQUAL(listener.stem(), "stem");
    BOOST_CHECK_EQUAL(listener.full(), "/base/stem");
    BOOST_CHECK_EQUAL(listener.polling(), static_cast<uint32_t>(2));
    BOOST_CHECK_EQUAL(listener.revision(), static_cast<uint64_t>(0));
}

BOOST_AUTO_TEST_CASE(can_create_parameterised_configured_listener_with_placeholders) {
    using namespace ecf::service::aviso;
    ConfiguredListener listener("http://unknown:1234", "path", "name", "/{a}", "{b}/{c}", 2, 0);
    listener.with_parameter("a", "aaa");
    listener.with_parameter("b", "bbb");
    listener.with_parameter("c", "ccc");
    BOOST_CHECK_EQUAL(listener.address(), "http://unknown:1234");
    BOOST_CHECK_EQUAL(listener.path(), "path");
    BOOST_CHECK_EQUAL(listener.name(), "name");
    BOOST_CHECK_EQUAL(listener.base(), "/{a}");
    BOOST_CHECK_EQUAL(listener.resolved_base(), "/aaa");
    BOOST_CHECK_EQUAL(listener.prefix(), "/aaa/");
    BOOST_CHECK_EQUAL(listener.stem(), "{b}/{c}");
    BOOST_CHECK_EQUAL(listener.full(), "/{a}/{b}/{c}");
    BOOST_CHECK_EQUAL(listener.polling(), static_cast<uint32_t>(2));
    BOOST_CHECK_EQUAL(listener.revision(), static_cast<uint64_t>(0));
}

BOOST_AUTO_TEST_CASE(can_detect_non_matching_key) {
    using namespace ecf::service::aviso;
    ConfiguredListener listener("http://unknown:1234", "path", "name", "/{a}", "{b}/{c}", 2, 0);
    listener.with_parameter("a", "aaa");
    listener.with_parameter("b", "bbb");
    listener.with_parameter("c", 0);

    auto match = listener.accepts("non-matching-key", "value", 0);
    BOOST_CHECK(!match.has_value());
}

BOOST_AUTO_TEST_CASE(can_detect_matching_key) {
    using namespace ecf::service::aviso;
    ConfiguredListener listener("http://unknown:1234", "path", "name", "/{a}", "{b}/{c}/{d}/{e}", 2, 0);
    listener.with_parameter("a", "aaa");
    listener.with_parameter("b", "bbb");
    listener.with_parameter("c", 0);
    listener.with_parameter("e", std::vector<std::string>{"valid1", "valid2"});

    {
        auto match = listener.accepts("/aaa/bbb/0/dontcare/valid1", "value", 1);
        BOOST_CHECK(match.has_value());
        BOOST_CHECK_EQUAL(match.value().key(), "/aaa/bbb/0/dontcare/valid1");
        BOOST_CHECK_EQUAL(match.value().value(), "value");
        BOOST_CHECK_EQUAL(match.value().revision(), static_cast<uint64_t>(1));
    }
    {
        auto match = listener.accepts("/aaa/bbb/0/dontcare/valid2", "value", 2);
        BOOST_CHECK(match.has_value());
        BOOST_CHECK_EQUAL(match.value().key(), "/aaa/bbb/0/dontcare/valid2");
        BOOST_CHECK_EQUAL(match.value().value(), "value");
        BOOST_CHECK_EQUAL(match.value().revision(), static_cast<uint64_t>(2));
    }
    {
        auto match = listener.accepts("/aaa/bbb/0/dontcare/invalid", "value", 3);
        BOOST_CHECK(!match.has_value());
    }
}

BOOST_AUTO_TEST_CASE(can_print_configured_listener) {
    using namespace ecf::service::aviso;
    ConfiguredListener listener("http://unknown:1234", "path", "name", "/{a}", "{b}/{c}", 2, 0);
    listener.with_parameter("a", "aaa");
    listener.with_parameter("b", "bbb");
    listener.with_parameter("c", "ccc");

    std::ostringstream oss;
    oss << listener;
    BOOST_CHECK(oss.str().find("ConfiguredListener") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_make_configured_listener) {
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

    BOOST_CHECK_EQUAL(listener.path(), path);
    BOOST_CHECK_EQUAL(listener.name(), "dissemination");
    BOOST_CHECK_EQUAL(listener.address(), address);
    BOOST_CHECK_EQUAL(listener.base(), "/ec/diss/{destination}");
    BOOST_CHECK_EQUAL(listener.stem(),
                      "date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={"
                      "stream},step={step}");
    BOOST_CHECK_EQUAL(listener.polling(), static_cast<uint32_t>(60));
    BOOST_CHECK_EQUAL(listener.revision(), static_cast<uint64_t>(1));
}

BOOST_AUTO_TEST_CASE(can_update_revision_in_configured_listener) {
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

    BOOST_CHECK_EQUAL(listener.revision(), static_cast<uint64_t>(1));

    listener.update_revision(2);

    BOOST_CHECK_EQUAL(listener.revision(), static_cast<uint64_t>(2));
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_listener_configuration_with_invalid_configuration_content) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
            {
                INVALID CONTENT
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

    BOOST_CHECK_EXCEPTION(
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision),
        std::runtime_error,
        [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Failed to parse listener configuration") != std::string::npos;
        });
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_listener_configuration_without_event_specification) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
            {
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

    BOOST_CHECK_EXCEPTION(
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision),
        std::runtime_error,
        [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Listener configuration does not specify 'event'") != std::string::npos;
        });
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_listener_configuration_without_request_specification) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
            {
                "event": "dissemination"
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

    BOOST_CHECK_EXCEPTION(
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision),
        std::runtime_error,
        [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Listener configuration does not specify 'request'") != std::string::npos;
        });
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_listener_configuration_with_invalid_schema_content) {
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
                INVALID CONTENT
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

    BOOST_CHECK_EXCEPTION(
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision),
        std::runtime_error,
        [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Failed to load listener schema") != std::string::npos;
        });
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_listener_configuration_with_invalid_event_kind) {
    using namespace ecf::service::aviso;

    std::string path           = "/s1/f1/t1";
    std::string listener_cfg   = R"(
            {
                "event": "invalid_kind_event",
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

    BOOST_CHECK_EXCEPTION(
        ConfiguredListener::make_configured_listener(path, listener_cfg, address, schema_content, polling, revision),
        std::runtime_error,
        [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Listener could not be found in schema") != std::string::npos;
        });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_AvisoSchema)

BOOST_AUTO_TEST_CASE(can_load_aviso_schema_from_string) {
    using namespace ecf::service::aviso;
    std::string schema_content = R"(
            {
                "dissemination": {
                    "endpoint":[
                        {
                            "engine":[
                                "etcd_rest"
                            ],
                            "base":"/ec/diss/{destination}",
                            "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                            "admin":"/ec/admin/{date}/{destination}"
                        }
                    ]
                },
                "mars": {
                    "endpoint":[
                        {
                            "engine":[
                                "etcd_rest"
                            ],
                            "base":"/ec/mars",
                            "stem":"date={date},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}"
                        }
                    ]
                }
            }
        )";

    ListenerSchema schema = ListenerSchema::load_from_string(schema_content);
    BOOST_CHECK(schema.get_listener("dissemination").has_value());
    BOOST_CHECK(schema.get_listener("mars").has_value());
    BOOST_CHECK(!schema.get_listener("invalid").has_value());
}

BOOST_AUTO_TEST_CASE(can_handle_invalid_schema_content) {
    using namespace ecf::service::aviso;
    std::string schema_content = R"(
            {
                INVALID CONTENT
                "dissemination": {
                    "endpoint":[
                        {
                            "engine":[
                                "etcd_rest"
                            ],
                            "base":"/ec/diss/{destination}",
                            "stem":"date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}",
                            "admin":"/ec/admin/{date}/{destination}"
                        }
                    ]
                },

            }
        )";

    BOOST_CHECK_EXCEPTION(
        ListenerSchema::load_from_string(schema_content), std::runtime_error, [](const std::exception& e) {
            auto reason = std::string(e.what());
            return reason.find("Failed to parse listener schema") != std::string::npos;
        });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
