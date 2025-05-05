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
    #define CPPHTTPLIB_OPENSSL_SUPPORT 1
#endif

#define CPPHTTPLIB_THREAD_POOL_COUNT 1
#define CPPHTTPLIB_ZLIB_SUPPORT 1

#include <httplib.h>

#include <boost/test/unit_test.hpp>

#include "Certificate.hpp"
#include "InvokeServer.hpp"
#include "TokenFile.hpp"
#include "ecflow/http/HttpServer.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/JSON.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(S_Http)

BOOST_AUTO_TEST_SUITE(T_ApiV1)

using ecf::http::HttpServer;
using ecf::http::HttpStatusCode;
using ecf::http::ojson;

const std::string API_HOST("localhost");
const std::string API_KEY("3a8c3f7ac204d9c6370b5916bd8b86166c208e10776285edcbc741d56b5b4c1e");

std::unique_ptr<Certificate> create_certificate() {
    auto cert_dir = ecf::environment::fetch("ECF_API_CERT_DIRECTORY");

    const std::string path_to_cert = (cert_dir) ? cert_dir.value() : ecf::environment::get("HOME") + "/.ecflowrc/ssl/";

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
    std::string tokens_file = cwd.string() + "/api-tokens.json";

    auto tokenfile = std::make_unique<TokenFile>(tokens_file);
    BOOST_TEST_MESSAGE("Token file " << tokens_file);
    return tokenfile;
}

void start_api_server() {
    if (ecf::environment::has("NO_API_SERVER")) {
        return; // terminate early, for debugging purposes
    }

    std::thread t([] {
#if defined(ECF_TEST_HTTP_BACKEND)
        char* argv[] = {(char*)"ecflow_http",
                        (char*)"-v",
                        (char*)"--polling_interval",
                        (char*)"1",
                        (char*)"--port",
                        (char*)"8081",
                        (char*)"--http",
                        NULL};
        int argc     = 7;
#else
        char* argv[] = {(char*)"ecflow_http",
                        (char*)"-v",
                        (char*)"--polling_interval",
                        (char*)"1",
                        (char*)"--port",
                        (char*)"8080",
                        NULL};
        int argc     = 6;
#endif

        HttpServer server(argc, argv);
        server.run();
    });

    sleep(1);
    t.detach();
}

std::unique_ptr<InvokeServer> start_ecflow_server() {
    if (ecf::environment::has("NO_ECFLOW_SERVER")) {
        return nullptr;
    }

    bool use_http_backend = false;
#if defined(ECF_TEST_HTTP_BACKEND)
    use_http_backend = true;
#endif

    auto srv = std::make_unique<InvokeServer>(use_http_backend);

    auto port = ecf::environment::get("ECF_PORT");
    BOOST_REQUIRE_MESSAGE(srv->server_started, "Server failed to start on port " << port);
    BOOST_TEST_MESSAGE("ecflow server at localhost:" << port);

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

#if defined(ECF_TEST_HTTP_BACKEND)
        setenv("ECF_PORT", "3198", 0);
#else
        setenv("ECF_PORT", "3199", 0);
#endif
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

httplib::Result request(const std::string& method,
                        const std::string& resource,
                        const std::string& payload             = "",
                        const std::string& token               = "",
                        const httplib::Headers& custom_headers = {}) {
#if defined(ECF_TEST_HTTP_BACKEND)
    httplib::SSLClient c(API_HOST, 8081);
#else
    httplib::SSLClient c(API_HOST, 8080);
#endif

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

bool check_for_path(const std::string& path) {
    try {
        handle_response(request("head", path), HttpStatusCode::success_ok, true);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool check_for_element(const std::string& path,
                       const std::string& key_name,
                       const std::string& attr_name,
                       const std::string& value) {

    try {
        auto r = handle_response(request("get", path), HttpStatusCode::success_ok, true);
        auto j = ojson::parse(r.body);

        if (j.is_null()) {
            return false;
        }
        else if (j.is_array() == false) {
            j = ojson::array({j});
            ECF_TEST_DBG(<< "json is " << j);
        }

        for (const auto& x : j) {
            if (attr_name.empty() == false) {
                // { "name": "foo", "value": "bar"}
                if (attr_name == x["name"] && value == ecf::http::json_type_to_string(x[key_name])) {
                    return true;
                }
            }
            else if (key_name.empty() == false) {
                // { "foo" : "bar" }
                if (ecf::http::json_type_to_string(x[key_name]) == value) {
                    return true;
                }
            }
            else {
                // "bar"
                if (value == ecf::http::json_type_to_string(x)) {
                    return true;
                }
            }
        }
    }
    catch (const std::exception& e) {
        BOOST_TEST_MESSAGE(e.what());
    }
    return false;
}

BOOST_AUTO_TEST_CASE(test_server) {
    ECF_NAME_THIS_TEST();

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
    ECF_NAME_THIS_TEST();

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

    auto content = ojson::parse(result.body);
    bool found   = false;

    for (const auto& suite : content) {
        if (suite.get<std::string>() == "test") {
            found = true;
            break;
        }
    }
    BOOST_REQUIRE(found);

    result  = handle_response(request("get", "/v1/suites/tree"));
    content = ojson::parse(result.body);
    found   = false;

    for (const auto& suite : content.items()) {
        if (suite.key() == "test") {
            found = true;
            break;
        }
    }

    BOOST_REQUIRE(found);

    result  = handle_response(request("get", "/v1/suites/test/a/tree"));
    content = ojson::parse(result.body);
    found   = false;

    for (const auto& node : content.items()) {
        if (node.key() == "a") {
            found = true;
            break;
        }
    }

    BOOST_REQUIRE(found);

    handle_response(request("put", "/v1/suites/test/status", R"({"action":"begin"})", API_KEY));

    auto response = ojson::parse(handle_response(request("get", "/v1/suites/test/definition")).body);

    const std::string& correct = "suite test\n  family a\n    task a\n  endfamily\nendsuite\n";

    BOOST_REQUIRE(response.at("definition").get<std::string>() == correct);

    handle_response(request("put", "/v1/suites/test/status", R"({"action":"suspend"})", API_KEY));
    wait_until([] { return check_for_element("/v1/suites/test/status?filter=status", "", "", "suspended"); });
}

BOOST_AUTO_TEST_CASE(test_node_basic_tree) {
    ECF_NAME_THIS_TEST();

    // (0) Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/basic_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/basic_suite/definition"); });

    // (1) Publish 'basic_suite' suite

    std::string suite_definition =
        R"({"definition" : "suite basic_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/basic_suite/definition"); });

    // (2) Retrieve 'basic_suite' suite tree
    {
        auto result  = handle_response(request("get", "/v1/suites/tree"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("basic_suite"));
        BOOST_REQUIRE(content["basic_suite"].contains("f"));
        BOOST_REQUIRE(content["basic_suite"]["f"].contains("t"));
    }

    // (3) Retrieve 'basic_suite' suite tree, explicitly specifying basic content
    {
        auto result  = handle_response(request("get", "/v1/suites/tree?content=basic"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("basic_suite"));
        BOOST_REQUIRE(content["basic_suite"].contains("f"));
        BOOST_REQUIRE(content["basic_suite"]["f"].contains("t"));
    }

    // (4) Retrieve specific node tree
    {
        auto result  = handle_response(request("get", "/v1/suites/basic_suite/f/tree?content=basic"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("f"));
        BOOST_REQUIRE(content["f"].contains("t"));
    }

    // (5) Clean up
    request("delete", "/v1/suites/basic_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/basic_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_node_full_tree) {
    ECF_NAME_THIS_TEST();

    // (0) Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });

    // (1) Publish 'full_tree' suite

    std::string suite_definition =
        R"({"definition" : "suite full_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/full_suite/definition"); });

    // (2) Retrieve 'full_suite' suite tree, explicitly specifying full content
    {
        auto result  = handle_response(request("get", "/v1/suites/tree?content=full"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("full_suite"));
        BOOST_REQUIRE(content["full_suite"].contains("type"));
        BOOST_REQUIRE(content["full_suite"]["type"] == "suite");
        BOOST_REQUIRE(content["full_suite"].contains("state"));
        BOOST_REQUIRE(content["full_suite"]["state"].contains("node"));
        BOOST_REQUIRE(content["full_suite"]["state"].contains("default"));
        BOOST_REQUIRE(content["full_suite"].contains("children"));

        BOOST_REQUIRE(content["full_suite"]["children"].contains("f"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"].contains("type"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["type"] == "family");
        BOOST_REQUIRE(content["full_suite"]["children"]["f"].contains("state"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["state"].contains("node"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["state"].contains("default"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"].contains("children"));

        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"].contains("t"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"].contains("type"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"]["type"] == "task");
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"].contains("state"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"]["state"].contains("node"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"]["state"].contains("default"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"].contains("attributes"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"]["attributes"].size() == 3);
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"].contains("aliases"));
        BOOST_REQUIRE(content["full_suite"]["children"]["f"]["children"]["t"]["aliases"].size() == 0);
    }

    // (2) Retrieve 'full_suite' suite tree, explicitly specifying full content
    {
        auto result  = handle_response(request("get", "/v1/suites/full_suite/f/tree?content=full"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("f"));
        BOOST_REQUIRE(content["f"].contains("state"));
        BOOST_REQUIRE(content["f"]["state"].contains("node"));
        BOOST_REQUIRE(content["f"]["state"].contains("default"));
        BOOST_REQUIRE(content["f"].contains("children"));

        BOOST_REQUIRE(content["f"]["children"].contains("t"));
        BOOST_REQUIRE(content["f"]["children"]["t"].contains("state"));
        BOOST_REQUIRE(content["f"]["children"]["t"]["state"].contains("node"));
        BOOST_REQUIRE(content["f"]["children"]["t"]["state"].contains("default"));
        BOOST_REQUIRE(content["f"]["children"]["t"].contains("attributes"));
        BOOST_REQUIRE(content["f"]["children"]["t"]["attributes"].size() == 3);
        BOOST_REQUIRE(content["f"]["children"]["t"].contains("aliases"));
        BOOST_REQUIRE(content["f"]["children"]["t"]["aliases"].size() == 0);
    }

    // (4) Clean up
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_node_full_tree_with_generated_variables) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    // (0) Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });

    // (1) Publish 'full_tree' suite

    std::string suite_definition =
        R"({"definition" : "suite full_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/full_suite/definition"); });

    // (2) Retrieve 'full_suite' suite tree, explicitly requesting generated variables
    {
        auto result  = handle_response(request("get", "/v1/suites/full_suite/f/tree?content=full&gen_vars=true"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("f"));
        // Check family attributes
        BOOST_REQUIRE(content["f"].contains("attributes"));
        BOOST_REQUIRE(content["f"]["attributes"].size() == 2);
        {
            size_t count = 0;
            for (const auto& attr : content["f"]["attributes"]) {
                BOOST_REQUIRE(attr.contains("type"));
                if (attr["type"] == "variable") {
                    BOOST_REQUIRE(attr.contains("name"));
                    BOOST_REQUIRE(attr.contains("value"));
                    if (attr.contains("generated")) {
                        BOOST_REQUIRE(attr["generated"] == true);
                        ++count;
                    }
                }
            }
            BOOST_REQUIRE(count >= 2);
        }
        BOOST_REQUIRE(content["f"].contains("children"));
        BOOST_REQUIRE(content["f"]["children"].contains("t"));
        // Check task attributes
        BOOST_REQUIRE(content["f"]["children"]["t"].contains("attributes"));
        BOOST_REQUIRE(content["f"]["children"]["t"]["attributes"].size() == 11);
        {
            size_t count = 0;
            for (const auto& attr : content["f"]["children"]["t"]["attributes"]) {
                BOOST_REQUIRE(attr.contains("type"));
                if (attr["type"] == "variable") {
                    BOOST_REQUIRE(attr.contains("name"));
                    BOOST_REQUIRE(attr.contains("value"));
                    if (attr.contains("generated")) {
                        BOOST_REQUIRE(attr["generated"] == true);
                        ++count;
                    }
                }
            }
            BOOST_REQUIRE(count >= 2);
        }
    }

    // (4) Clean up
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_token_authentication) {
    ECF_NAME_THIS_TEST();

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

    const std::string pbkdf2_API_KEY("351db772d94310a6d57aa7144448f4c108e7ee2e2a00a74edbdf8edb11bee71b");
    handle_response(
        request("post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", pbkdf2_API_KEY),
        HttpStatusCode::success_created);
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    const std::string expired_API_KEY("764073a74875ada28859454e58881229a5149ae400589fc617234d8d96c6d91a");
    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", expired_API_KEY),
        HttpStatusCode::client_error_unauthorized);

    const std::string revoked_API_KEY("5c6f6a003f3292c4d7671c9ad8ca10fb76e2273e584992f0d1f8fdf4abcdc81e");
    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", revoked_API_KEY),
        HttpStatusCode::client_error_unauthorized);
}

BOOST_AUTO_TEST_CASE(test_family_add, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_suite")) {
    ECF_NAME_THIS_TEST();

    handle_response(
        request("put", "/v1/suites/test/definition", R"({"definition": "family dynamic\nendfamily"})", API_KEY));
    wait_until([] { return check_for_path("/v1/suites/test/dynamic/definition"); });
}

// STATUS

BOOST_AUTO_TEST_CASE(test_status, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

    const std::map<std::string, std::string> statuses{
        {"abort", "aborted"}, {"complete", "complete"}, {"requeue", "queued"}, {"suspend", "suspended"}};

    for (const auto& status : statuses) {
        ojson j = {{"action", status.first}};
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

BOOST_AUTO_TEST_CASE(test_variable, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_meter, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_limit, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_event, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_label, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_time, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_day, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_date, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_today, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_cron, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_late, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_complete, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_autocancel, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_autoarchive, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_autorestore, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_CASE(test_output, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

    handle_response(request("get", "/v1/suites/test/a/a/output"), HttpStatusCode::client_error_not_found);
}

// SCRIPT

BOOST_AUTO_TEST_CASE(test_script, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_family_add")) {
    ECF_NAME_THIS_TEST();

    handle_response(request("get", "/v1/suites/test/a/a/script"), HttpStatusCode::client_error_not_found);
}

// DELETE FAMILY

BOOST_AUTO_TEST_CASE(test_suite_family_delete, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_autorestore")) {
    ECF_NAME_THIS_TEST();

    handle_response(request("delete", "/v1/suites/test/dynamic/definition", "", API_KEY),
                    HttpStatusCode::success_no_content);
    wait_until([] { return false == check_for_path("/v1/suites/test/dynamic/definition"); });
    wait_until([] { return check_for_path("/v1/suites/test/definition"); });

    handle_response(request("delete", "/v1/suites/test/definition", "", API_KEY), HttpStatusCode::success_no_content);
    wait_until([] { return false == check_for_path("/v1/suites/test/definition"); });
}

BOOST_AUTO_TEST_CASE(test_statistics, *boost::unit_test::depends_on("S_Http/T_ApiV1/test_server")) {
    ECF_NAME_THIS_TEST();

    auto response = handle_response(request("get", "/v1/statistics"));
    auto j        = ojson::parse(response.body);

    BOOST_REQUIRE(j["num_requests"].get<int>() > 0);
    BOOST_REQUIRE(j["num_errors"].get<int>() > 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
