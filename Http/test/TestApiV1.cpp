/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifdef ECF_OPENSSL
    #define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <boost/test/unit_test.hpp>

#include "Certificate.hpp"
#include "InvokeServer.hpp"
#include "TokenFile.hpp"
#include "ecflow/http/HttpServer.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "httplib.h"

BOOST_AUTO_TEST_SUITE(HttpTestSuite)

namespace utf = boost::unit_test;
using json    = nlohmann::json;
using string  = std::string;

const string API_HOST("localhost");
const string API_KEY("3a8c3f7ac204d9c6370b5916bd8b86166c208e10776285edcbc741d56b5b4c1e");

std::unique_ptr<Certificate> create_certificate() {
    const char* cert_dir = getenv("ECF_API_CERT_DIRECTORY");
    const std::string path_to_cert =
        (cert_dir == nullptr) ? std::string(getenv("HOME")) + "/.ecflowrc/ssl/" : std::string(cert_dir);

    std::unique_ptr<Certificate> cert;

    BOOST_TEST_MESSAGE("Certificates at " << path_to_cert);

    if (fs::exists(path_to_cert + "/server.crt") == false || fs::exists(path_to_cert + "/server.key") == false) {
        if (fs::exists(path_to_cert) == false) {
            fs::create_directories(path_to_cert);
        }

        cert = std::make_unique<Certificate>(path_to_cert);

        setenv("ECF_API_CERT_DIRECTORY", path_to_cert.c_str(), 1);
        return cert;
    }
    return cert;
}

std::unique_ptr<TokenFile> create_token_file() {
    fs::path cwd(fs::current_path());
    string tokens_file = cwd.string() + "/api-tokens.json";

    auto tokenfile = std::make_unique<TokenFile>(tokens_file);
    BOOST_TEST_MESSAGE("Token file " << tokens_file);
    return tokenfile;
}

void start_api_server() {

    if (getenv("NO_API_SERVER") != nullptr)
        return; // for debugging
    std::thread t([] {
        int argc     = 3;
        char* argv[] = {(char*)"ecflow_http", (char*)"--polling_interval", (char*)"1", NULL};

        HttpServer server(argc, argv);
        server.run();
    });

    sleep(1);
    t.detach();
}

std::unique_ptr<InvokeServer> start_ecflow_server() {
    if (getenv("NO_ECFLOW_SERVER") != nullptr)
        return nullptr;

    auto srv = std::make_unique<InvokeServer>();

    BOOST_REQUIRE_MESSAGE(srv->server_started, "Server failed to start on port " << getenv("ECF_PORT"));
    BOOST_TEST_MESSAGE("ecflow server at localhost:" << getenv("ECF_PORT"));
    return srv;
}

struct SetupTest
{
    SetupTest() : tokenfile(create_token_file()), cert(create_certificate()) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);

        if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
            throw std::runtime_error("Failed to set signal mask");
        }

        setenv("ECF_PORT", "3199", 0);
        setenv("ECF_HOST", "localhost", 1);
    }
    void setup() {
        // This needs to be initialized in setup() instead
        // of constructor... really don't know why
        srv = start_ecflow_server();
        start_api_server();
        printf("======= TESTS STARTING ========\n");
    }
    ~SetupTest() { printf("======= TESTS FINISHED ========\n"); }

    std::unique_ptr<TokenFile> tokenfile{nullptr};
    std::unique_ptr<Certificate> cert{nullptr};
    std::unique_ptr<InvokeServer> srv{nullptr};
};

BOOST_TEST_GLOBAL_FIXTURE(SetupTest);

httplib::Response handle_response(const httplib::Result& r,
                                  HttpStatusCode expected_code = HttpStatusCode::success_ok,
                                  bool throw_on_error          = false) {
    if (r) {
        BOOST_TEST_MESSAGE("Return code: " << r->status);
        BOOST_TEST_MESSAGE("Return value: " << r->body);
    }
    else {
        BOOST_TEST_MESSAGE("Null reply from server");
    }

    if (throw_on_error) {
        if (!r) {
            throw std::runtime_error("NULL reply from server");
        }
        else if (r->status != expected_code) {
            throw std::runtime_error(
                "Expected status code: " + ecf::convert_to<std::string>(static_cast<int>(expected_code)) +
                " got: " + ecf::convert_to<std::string>(r->status));
        }
    }
    else {
        BOOST_REQUIRE_MESSAGE(r, "ERROR: no response");
        BOOST_REQUIRE_MESSAGE(r->status == expected_code,
                              "ERROR: status code is not " +
                                  ecf::convert_to<std::string>(static_cast<int>(expected_code)));
    }

    return httplib::Response(*r);
}

httplib::Result request(const string& method,
                        const string& resource,
                        const string& payload                  = "",
                        const string& token                    = "",
                        const httplib::Headers& custom_headers = {}) {
    httplib::SSLClient c(API_HOST, 8080);

    c.enable_server_certificate_verification(false);
    c.set_connection_timeout(3);

    BOOST_TEST_MESSAGE("Request URL: " << method << " " << API_HOST << resource);
    BOOST_TEST_MESSAGE("Request body: " << payload);

    httplib::Headers h = {{"Content-type", "application/json"}};

    if ((method == "post" || method == "put" || method == "delete") && token.empty() == false) {
        h.insert(std::make_pair("Authorization", "Bearer " + token));
    }

    for (const auto& kv : custom_headers) {
        h.insert(kv);
    }

    c.set_default_headers(h);

    if (method == "get" || method == "head")
        return c.Get(resource);
    else if (method == "post")
        return c.Post(resource, payload, "application/json");
    else if (method == "put")
        return c.Put(resource, payload, "application/json");
    else if (method == "delete")
        return c.Delete(resource, payload, "application/json");

    BOOST_FAIL("Unknown HTTP method");

    return httplib::Result(nullptr, httplib::Error::Unknown); // to silence compiler warnings
}

template <typename T>
bool wait_until(T&& func, int wait_time = 1, int wait_count = 10) {
    int c = 0;

    while (func() == false) {
        c++;
        if (c > wait_count) {
            BOOST_FAIL("Test failed");
        }
        sleep(wait_time);
    }
    return true;
}

bool check_for_path(const string& path) {
    try {
        handle_response(request("head", path), HttpStatusCode::success_ok, true);
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string json_type_to_string(const json& j) {
    switch (j.type()) {
        case json::value_t::null:
            return "null";
        case json::value_t::boolean:
            return (j.get<bool>()) ? "true" : "false";
        case json::value_t::string:
            return j.get<std::string>();
        case json::value_t::binary:
            return j.dump();
        case json::value_t::array:
        case json::value_t::object:
            return j.dump();
        case json::value_t::discarded:
            return "discarded";
        case json::value_t::number_integer:
            return ecf::convert_to<std::string>(j.get<int>());
        case json::value_t::number_unsigned:
            return ecf::convert_to<std::string>(j.get<unsigned int>());
        case json::value_t::number_float:
            return std::to_string(j.get<double>());
        default:
            return std::string();
    }
}

bool check_for_element(const string& path, const string& key_name, const string& attr_name, const string& value) {

    try {
        auto r = handle_response(request("get", path), HttpStatusCode::success_ok, true);
        auto j = json::parse(r.body);

        if (j.is_null()) {
            return false;
        }
        else if (j.is_array() == false) {
            j = json::array({j});
            std::cout << "json is " << j << std::endl;
        }

        for (const auto& x : j) {
            // std::cout << x << std::endl;
            if (attr_name.empty() == false) {
                // { "name": "foo", "value": "bar"}
                // std::cout << "expecting: " << attr_name << "=" << value << " got: " << x["name"].get<std::string>()
                // << "=" << json_type_to_string(x[key_name]) << std::endl;
                if (attr_name == x["name"] && value == json_type_to_string(x[key_name]))
                    return true;
            }
            else if (key_name.empty() == false) {
                // { "foo" : "bar" }
                // std::cout << "expecting: " << key_name << "='" << value << " got: " << key_name << "='" <<
                // json_type_to_string(x[key_name]) << "'" << std::endl;

                if (json_type_to_string(x[key_name]) == value)
                    return true;
            }
            else {
                // "bar"
                // std::cout << "expecting: '" << value << "' got: '" << json_type_to_string(x) << "'" << std::endl;

                if (value == json_type_to_string(x))
                    return true;
            }
        }
    }
    catch (const std::exception& e) {
        BOOST_TEST_MESSAGE(e.what());
    }
    // BOOST_TEST_MESSAGE("Attribute " << attr_name << " --> " << key_name << "=" << value << " at path " << path << "
    // not found");
    return false;
}

BOOST_AUTO_TEST_CASE(test_server) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    wait_until([] { return check_for_element("/v1/server/status?filter=status", "", "", "RUNNING"); });

    handle_response(request("get", "/v1/server/ping"));

    handle_response(
        request("post", "/v1/server/attributes", R"({"type":"variable","name":"foo","value":"bar"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "foo", "bar"); });

    handle_response(
        request("put", "/v1/server/attributes", R"({"type":"variable","name":"foo","value":"baz"})", API_KEY));
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "foo", "baz"); });

    handle_response(request("delete", "/v1/server/attributes", R"({"type":"variable","name":"foo"})", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until(
        [] { return false == check_for_element("/v1/server/attributes?filter=variables", "value", "foo", "bar"); });
}

BOOST_AUTO_TEST_CASE(test_suite) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    // remove test-suite if it exists; disregard any problems with the call
    request("delete", "/v1/suites/test/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/test/definition"); });

    handle_response(request("post",
                            "/v1/suites",
                            R"({"definition" : "suite test\n  family a\n    task a\n  endfamily\nendsuite"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/test/definition"); });

    auto result = handle_response(request("get", "/v1/suites"));

    json content = json::parse(result.body);
    bool found   = false;

    for (const auto& suite : content) {
        if (suite.get<string>() == "test") {
            found = true;
            break;
        }
    }
    BOOST_REQUIRE(found);

    result  = handle_response(request("get", "/v1/suites/tree"));
    content = json::parse(result.body);
    found   = false;

    for (const auto& suite : content.items()) {
        if (suite.key() == "test") {
            found = true;
            break;
        }
    }

    BOOST_REQUIRE(found);

    result  = handle_response(request("get", "/v1/suites/test/a/tree"));
    content = json::parse(result.body);
    found   = false;

    for (const auto& node : content.items()) {
        if (node.key() == "a") {
            found = true;
            break;
        }
    }

    BOOST_REQUIRE(found);

    handle_response(request("put", "/v1/suites/test/status", R"({"action":"begin"})", API_KEY));

    auto response = json::parse(handle_response(request("get", "/v1/suites/test/definition")).body);

    const std::string& correct = "suite test\n  family a\n    task a\n  endfamily\nendsuite\n";

    // std::cout << "correct: " << correct << std::endl
    //           << "response:" << response.at("definition").get<string>() << std::endl;

    BOOST_REQUIRE(response.at("definition").get<string>() == correct);

    handle_response(request("put", "/v1/suites/test/status", R"({"action":"suspend"})", API_KEY));
    wait_until([] { return check_for_element("/v1/suites/test/status?filter=status", "", "", "suspended"); });
}

BOOST_AUTO_TEST_CASE(test_token_authentication) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    handle_response(request("put",
                            "/v1/server/attributes",
                            R"({"type":"variable","name":"xfoo","value":"xbaz"})",
                            "",
                            {{"X-API-Key", API_KEY}}));
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    handle_response(
        request("delete", "/v1/server/attributes?key=" + API_KEY, R"({"type":"variable","name":"xfoo"})", ""),
        HttpStatusCode::success_no_content);
    wait_until(
        [] { return false == check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    const string pbkdf2_API_KEY("351db772d94310a6d57aa7144448f4c108e7ee2e2a00a74edbdf8edb11bee71b");
    handle_response(
        request("post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", pbkdf2_API_KEY),
        HttpStatusCode::success_created);
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    const string expired_API_KEY("764073a74875ada28859454e58881229a5149ae400589fc617234d8d96c6d91a");
    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", expired_API_KEY),
        HttpStatusCode::client_error_unauthorized);

    const string revoked_API_KEY("5c6f6a003f3292c4d7671c9ad8ca10fb76e2273e584992f0d1f8fdf4abcdc81e");
    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", revoked_API_KEY),
        HttpStatusCode::client_error_unauthorized);
}

BOOST_AUTO_TEST_CASE(test_family_add, *utf::depends_on("HttpTestSuite/test_suite")) {
    handle_response(
        request("put", "/v1/suites/test/definition", R"({"definition": "family dynamic\nendfamily"})", API_KEY));
    wait_until([] { return check_for_path("/v1/suites/test/dynamic/definition"); });
}

// STATUS

BOOST_AUTO_TEST_CASE(test_status, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    const std::map<std::string, std::string> statuses{
        {"abort", "aborted"}, {"complete", "complete"}, {"requeue", "queued"}, {"suspend", "suspended"}};

    for (const auto& status : statuses) {
        json j = {{"action", status.first}};
        handle_response(request("put", "/v1/suites/test/dynamic/status", j.dump(), API_KEY));

        wait_until(
            [&] { return check_for_element("/v1/suites/test/dynamic/status?filter=status", "", "", status.second); });
    }

    handle_response(request(
        "put", "/v1/suites/test/dynamic/status", R"({"action":"defstatus","defstatus_value":"complete"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/status?filter=default_status", "", "", "complete"); });

    handle_response(request("put", "/v1/suites/test/a/status", R"({"action":"archive"})", API_KEY));
    wait_until([] { return false == check_for_path("/v1/suites/test/a/a/definition"); });

    handle_response(request("put", "/v1/suites/test/a/status", R"({"action":"restore"})", API_KEY));
    wait_until([] { return check_for_path("/v1/suites/test/a/a/definition"); });
}

// VARIABLE

BOOST_AUTO_TEST_CASE(test_variable, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request(
            "post", "/v1/suites/test/dynamic/attributes", R"({"type":"variable","name":"foo","value":"bar"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=variables", "value", "foo", "bar"); });

    handle_response(request(
        "put", "/v1/suites/test/dynamic/attributes", R"({"type":"variable","name":"foo","value":"baz"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=variables", "value", "foo", "baz"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"variable","name":"foo"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=variables", "value", "foo", "baz");
    });
}

// METER

BOOST_AUTO_TEST_CASE(test_meter, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(request("post",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"meter","name":"foo","value":"10","min":"0","max":"20"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=meters", "value", "foo", "10"); });

    handle_response(
        request("put", "/v1/suites/test/dynamic/attributes", R"({"type":"meter","name":"foo","value":"15"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=meters", "value", "foo", "15"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"meter","name":"foo"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=meters", "value", "foo", "15");
    });
}

// LIMIT

BOOST_AUTO_TEST_CASE(test_limit, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"limit","name":"foo","value":"0"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=limits", "value", "foo", "0"); });

    handle_response(
        request("put", "/v1/suites/test/dynamic/attributes", R"({"type":"limit","name":"foo","value":"6"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=limits", "value", "foo", "6"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"limit","name":"foo"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=limits", "value", "foo", "6");
    });
}

// EVENT

BOOST_AUTO_TEST_CASE(test_event, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request(
            "post", "/v1/suites/test/dynamic/attributes", R"({"type":"event","name":"foo","value":"set"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=events", "value", "foo", "true"); });

    handle_response(request(
        "put", "/v1/suites/test/dynamic/attributes", R"({"type":"event","name":"foo","value":false})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=events", "value", "foo", "false"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"event","name":"foo"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=events", "value", "foo", "false");
    });
}

// LABEL

BOOST_AUTO_TEST_CASE(test_label, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request(
            "post", "/v1/suites/test/dynamic/attributes", R"({"type":"label","name":"foo","value":"bar"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=labels", "value", "foo", "bar"); });

    handle_response(request(
        "put", "/v1/suites/test/dynamic/attributes", R"({"type":"label","name":"foo","value":"baz"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=labels", "value", "foo", "baz"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"label","name":"foo"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=labels", "value", "foo", "baz");
    });
}

// TIME

BOOST_AUTO_TEST_CASE(test_time, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"time","value":"+00:20"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=times", "value", "", "+00:20"); });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"time","old_value":"+00:20","value":"+00:25"})",
                            API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=times", "value", "", "+00:25"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"time","value":"+00:25"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=times", "value", "", "+00:25");
    });
}

// DAY

BOOST_AUTO_TEST_CASE(test_day, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"day","value":"monday"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=days", "value", "", "monday"); });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"day","old_value":"monday","value":"tuesday"})",
                            API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=days", "value", "", "tuesday"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"day","value":"tuesday"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=days", "value", "", "tuesday");
    });
}

// DATE

BOOST_AUTO_TEST_CASE(test_date, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"date","value":"1.*.*"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=dates", "value", "", "1.*.*"); });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"date","old_value":"1.*.*","value":"2.*.*"})",
                            API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=dates", "value", "", "2.*.*"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"date","value":"2.*.*"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=dates", "value", "", "2.*.*");
    });
}

// TODAY

BOOST_AUTO_TEST_CASE(test_today, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"today","value":"03:00"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=todays", "value", "", "03:00"); });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"today","old_value":"03:00","value":"03:00 05:00 01:00"})",
                            API_KEY));
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=todays", "value", "", "03:00 05:00 01:00");
    });

    handle_response(
        request(
            "delete", "/v1/suites/test/dynamic/attributes", R"({"type":"today","value":"03:00 05:00 01:00"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false ==
               check_for_element("/v1/suites/test/dynamic/attributes?filter=todays", "value", "", "03:00 05:00 01:00");
    });
}

// CRON

BOOST_AUTO_TEST_CASE(test_cron, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"cron","value":"-w 0,1 10:00"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=crons", "value", "", "-w 0,1 10:00");
    });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"cron","old_value":"-w 0,1 10:00","value":"23:00"})",
                            API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=crons", "value", "", "23:00"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"cron","value":"23:00"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=crons", "value", "", "23:00");
    });
}

// LATE

BOOST_AUTO_TEST_CASE(test_late, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(request("post",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"late","value":"-s +00:01 -a 14:30 -c +00:01"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element(
            "/v1/suites/test/dynamic/attributes?filter=late", "value", "", "-s +00:01 -a 14:30 -c +00:01");
    });

    handle_response(request("put",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"late","old_value":"-s +00:01 -a 14:30 -c +00:01","value":"-c +00:01"})",
                            API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=late", "value", "", "-c +00:01"); });

    handle_response(
        request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"late","value":"-c +00:01"})", API_KEY),
        HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=late", "value", "", "-c +00:01");
    });
}

// COMPLETE

BOOST_AUTO_TEST_CASE(test_complete, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(request("post",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"complete","value":"/test/a eq complete"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element(
            "/v1/suites/test/dynamic/attributes?filter=complete", "value", "", "/test/a eq complete");
    });

    handle_response(request(
        "put", "/v1/suites/test/dynamic/attributes", R"({"type":"complete","value":"/test/a eq active"})", API_KEY));
    wait_until([] {
        return check_for_element(
            "/v1/suites/test/dynamic/attributes?filter=complete", "value", "", "/test/a eq active");
    });

    handle_response(request("delete",
                            "/v1/suites/test/dynamic/attributes",
                            R"({"type":"complete","value":"/test/a eq active"})",
                            API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=complete", "value", "", "");
    });
}

// AUTOCANCEL

BOOST_AUTO_TEST_CASE(test_autocancel, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"autocancel","value":"+01:00"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=autocancel", "value", "", "+01:00");
    });

    handle_response(
        request("put", "/v1/suites/test/dynamic/attributes", R"({"type":"autocancel","value":"0"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=autocancel", "value", "", "0"); });

    handle_response(request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"autocancel"})", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=autocancel", "value", "", "0");
    });
}

// AUTOARCHIVE

BOOST_AUTO_TEST_CASE(test_autoarchive, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"autoarchive","value":"+01:00"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=autoarchive", "value", "", "+01:00");
    });

    handle_response(
        request("put", "/v1/suites/test/dynamic/attributes", R"({"type":"autoarchive","value":"0"})", API_KEY));
    wait_until(
        [] { return check_for_element("/v1/suites/test/dynamic/attributes?filter=autoarchive", "value", "", "0"); });

    handle_response(request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"autoarchive"})", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] {
        return false == check_for_element("/v1/suites/test/dynamic/attributes?filter=autoarchive", "value", "", "");
    });
}

// AUTORESTORE

BOOST_AUTO_TEST_CASE(test_autorestore, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(
        request("post", "/v1/suites/test/dynamic/attributes", R"({"type":"autorestore","value":"/test/a"})", API_KEY),
        HttpStatusCode::success_created);
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=autorestore", "value", "", "/test/a");
    });

    handle_response(
        request("put", "/v1/suites/test/dynamic/attributes", R"({"type":"autorestore","value":"/test"})", API_KEY));
    wait_until([] {
        return check_for_element("/v1/suites/test/dynamic/attributes?filter=autorestore", "value", "", "/test");
    });

    handle_response(request("delete", "/v1/suites/test/dynamic/attributes", R"({"type":"autorestore"})", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] {
        return false ==
               check_for_element("/v1/suites/test/dynamic/attributes?filter=autorestore", "value", "", "/test");
    });
}

// OUTPUT

BOOST_AUTO_TEST_CASE(test_output, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;
    handle_response(request("get", "/v1/suites/test/a/a/output"), HttpStatusCode::client_error_not_found);
}

// SCRIPT

BOOST_AUTO_TEST_CASE(test_script, *utf::depends_on("HttpTestSuite/test_family_add")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;
    handle_response(request("get", "/v1/suites/test/a/a/script"), HttpStatusCode::client_error_not_found);
}

// DELETE FAMILY

BOOST_AUTO_TEST_CASE(test_suite_family_delete, *utf::depends_on("HttpTestSuite/test_autorestore")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    handle_response(request("delete", "/v1/suites/test/dynamic/definition", "", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] { return false == check_for_path("/v1/suites/test/dynamic/definition"); });
    wait_until([] { return check_for_path("/v1/suites/test/definition"); });

    handle_response(request("delete", "/v1/suites/test/definition", "", API_KEY), HttpStatusCode::success_no_content);
    wait_until([] { return false == check_for_path("/v1/suites/test/definition"); });
}

BOOST_AUTO_TEST_CASE(test_statistics, *utf::depends_on("HttpTestSuite/test_server")) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    auto response = handle_response(request("get", "/v1/statistics"));
    auto j        = json::parse(response.body);

    BOOST_REQUIRE(j["num_requests"].get<int>() > 0);
    BOOST_REQUIRE(j["num_errors"].get<int>() > 0);
}

BOOST_AUTO_TEST_SUITE_END()
