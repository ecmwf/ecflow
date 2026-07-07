/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <array>
#include <atomic>
#include <chrono>

#include <boost/test/unit_test.hpp>

#include "Certificate.hpp"
#include "InvokeServer.hpp"
#include "TokenFile.hpp"
#include "ecflow/core/HttpLibrary.hpp"
#include "ecflow/http/HttpServer.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/JSON.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(S_Http)

BOOST_AUTO_TEST_SUITE(T_ApiV1)

using ecf::http::HttpServer;
using ecf::http::HttpStatusCode;
using ecf::http::ojson;

const std::string API_HOST        = "localhost";
const std::string API_KEY         = TokenFile::generate_token();
const std::string API_KEY_pbkdf2  = TokenFile::generate_token();
const std::string API_KEY_expired = TokenFile::generate_token();
const std::string API_KEY_revoked = TokenFile::generate_token();

int get_random_port(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

#if defined(ECF_TEST_HTTP_BACKEND)
static const int ECF_TEST_HTTP_PORT_NR             = get_random_port(8000, 8999);
static const std::string ECF_TEST_HTTP_PORT        = std::to_string(ECF_TEST_HTTP_PORT_NR);
static const std::string ECF_TEST_HTTP_TOKENS_FILE = "api-tokens.using_http_backend.json";
#else
static const int ECF_TEST_HTTP_PORT_NR             = get_random_port(49152, 65535);
static const std::string ECF_TEST_HTTP_PORT        = std::to_string(ECF_TEST_HTTP_PORT_NR);
static const std::string ECF_TEST_HTTP_TOKENS_FILE = "api-tokens.using_tcpip_backend.json";
#endif

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
    std::string tokens_file = cwd.string() + ECF_TEST_HTTP_TOKENS_FILE;

    // Generate 4 random bearer tokens with different configurations
    std::vector<BearerToken> entries = {
        {API_KEY, "test-app-1", "", ""},                             // valid, no expiry
        {API_KEY_pbkdf2, "test-app-2", "2100-01-01T00:00:00Z", ""},  // valid, future expiry
        {API_KEY_expired, "test-app-3", "2000-01-01T00:00:00Z", ""}, // expired
        {API_KEY_revoked, "test-app-4", "", "2000-01-01T00:00:00Z"}, // revoked
    };

    auto tokenfile = std::make_unique<TokenFile>(tokens_file, entries);
    BOOST_TEST_MESSAGE("Token file " << tokens_file);
    return tokenfile;
}

static std::unique_ptr<std::thread> api_server;

///
/// @brief Flag to indicate readiness by the server thread once it successfully binds to the port
///
static std::atomic<bool> api_server_started{false};

void start_api_server() {
    if (ecf::environment::has("NO_API_SERVER")) {
        return; // terminate early, for debugging purposes
    }

    api_server_started = false;

    api_server = std::make_unique<std::thread>([] {
        std::string port = ECF_TEST_HTTP_PORT;
        fs::path cwd(fs::current_path());
        std::string tokens_file = cwd.string() + ECF_TEST_HTTP_TOKENS_FILE;

        // clang-format off
        std::array argv = {
            "ecflow_http"
            , "-v"
            , "--polling_interval", "1"
            , "--port", port.c_str()
            , "--tokens_file", tokens_file.c_str()
    #if defined(ECF_TEST_HTTP_BACKEND)
            , "--http"
    #endif
        };
        // clang-format on

        HttpServer server(argv.size(), const_cast<char**>(argv.data()));

        api_server_started = true; // Signal that server is starting (or has started)
        server.run();
        api_server_started = false; // Server is done (either normal shutdown or bind failure)
    });

    // Wait up to 10 seconds (100 * 100ms for the server thread to start/bind to port.
    for (int i = 0; i < 100 && !api_server_started; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // After waiting, if the server thread has not indicated that it started, fail the test immediately.
    if (!api_server_started) {
        BOOST_FAIL("REST API server thread failed to start within 10 seconds, when attenting to bind to port " +
                   ECF_TEST_HTTP_PORT);
        return;
    }

    // Give the server a moment to complete the bind and become ready.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

httplib::Result request(const std::string& method,
                        const std::string& resource,
                        const std::string& payload             = "",
                        const std::string& token               = "",
                        const httplib::Headers& custom_headers = {}) {
    httplib::SSLClient c(API_HOST, ECF_TEST_HTTP_PORT_NR);

    c.enable_server_certificate_verification(false);
    // Set connection timeout
    c.set_connection_timeout(3);
    // Set read and write timeouts
    // This prevents blocking indefinitely, after connection established, during the SSL handshake.
    c.set_read_timeout(5);
    c.set_write_timeout(5);

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

    if (method == "get" || method == "head") {
        return c.Get(resource);
    }
    else if (method == "post") {
        return c.Post(resource, payload, "application/json");
    }
    else if (method == "put") {
        return c.Put(resource, payload, "application/json");
    }
    else if (method == "delete") {
        return c.Delete(resource, payload, "application/json");
    }

    BOOST_FAIL("Unknown HTTP method");

    return httplib::Result(nullptr, httplib::Error::Unknown); // to silence compiler warnings
}

struct SetupTest
{
    SetupTest()
        : tokenfile(create_token_file()),
          cert(create_certificate()) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);

        if (pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0) {
            throw std::runtime_error("Failed to set signal mask");
        }

#if defined(ECF_TEST_HTTP_BACKEND)
        setenv("ECF_PORT", "3198", 0);
#else
        setenv("ECF_PORT", "3199", 0);
#endif
        setenv("ECF_HOST", "localhost", 1);

        // Ensure each separate test uses its own ECF_HOME...
        std::string ecf_home = (fs::current_path() / ("ecf_home_" + ecf::environment::get("ECF_PORT"))).string();
        fs::create_directories(ecf_home);
        setenv("ECF_HOME", ecf_home.c_str(), 1);
    }
    void setup() {
        // This needs to be initialized in setup() instead
        // of constructor... really don't know why
        srv = start_ecflow_server();
        start_api_server();
        printf("======= TESTS STARTING ========\n");
    }

    void teardown() {
        if (api_server && api_server->joinable()) {
            if (api_server_started) {
                // Request server to shut down gracefully.
                printf("Shutting down REST API\n");
                auto s = request("get", "/v1/shutdown");
                if (s && s->status != 200) {
                    BOOST_TEST_MESSAGE("WARNING: REST API shutdown request returned status " << s->status);
                }
                else if (!s) {
                    BOOST_TEST_MESSAGE("WARNING: REST API shutdown request received no response");
                }
            }
            printf("Waiting for REST API thread to finish\n");
            api_server->join();
        }

        printf("Shutting down Token file\n");
        tokenfile.reset();

        printf("Shutting down Certificate\n");
        cert.reset();

        printf("Shutting down ecFlow (main) server\n");
        srv.reset();
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
            throw std::runtime_error("nullptr reply from server");
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

template <typename F>
bool wait_until(F f, int wait_time = 1, int wait_count = 10) {
    int c = 0;

    while (f() == false) {
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
            ECF_TEST_DBG("json is " << j);
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

    // Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/basic_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/basic_suite/definition"); });

    // Publish 'basic_suite' suite

    std::string suite_definition =
        R"({"definition" : "suite basic_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/basic_suite/definition"); });

    // Retrieve 'basic_suite' suite tree
    {
        auto result  = handle_response(request("get", "/v1/suites/tree"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("basic_suite"));
        BOOST_REQUIRE(content["basic_suite"].contains("f"));
        BOOST_REQUIRE(content["basic_suite"]["f"].contains("t"));
    }

    // Retrieve 'basic_suite' suite tree, explicitly specifying basic content
    {
        auto result  = handle_response(request("get", "/v1/suites/tree?content=basic"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("basic_suite"));
        BOOST_REQUIRE(content["basic_suite"].contains("f"));
        BOOST_REQUIRE(content["basic_suite"]["f"].contains("t"));
    }

    // Retrieve specific node tree
    {
        auto result  = handle_response(request("get", "/v1/suites/basic_suite/f/tree?content=basic"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("f"));
        BOOST_REQUIRE(content["f"].contains("t"));
    }

    // Clean up
    request("delete", "/v1/suites/basic_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/basic_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_node_full_tree) {
    ECF_NAME_THIS_TEST();

    // Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });

    // Publish 'full_tree' suite

    std::string suite_definition =
        R"({"definition" : "suite full_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/full_suite/definition"); });

    // Retrieve 'full_suite' suite tree, explicitly specifying full content
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

    // Retrieve 'full_suite' suite tree, explicitly specifying full content
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

    // Clean up
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_node_full_tree_with_generated_variables) {
    std::cout << "======== " << boost::unit_test::framework::current_test_case().p_name << " =========" << std::endl;

    // Clean up -- in case there is any left-over from passed/failed tests
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });

    // Publish 'full_tree' suite

    std::string suite_definition =
        R"({"definition" : "suite full_suite\n  family f\n    task t\n      label l \"value\"\n      meter m 0 100 50\n      event e\n  endfamily\nendsuite\n# comment"})";
    handle_response(request("post", "/v1/suites", suite_definition, API_KEY), HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/full_suite/definition"); });

    // Retrieve 'full_suite' suite tree, explicitly requesting generated variables
    {
        auto result  = handle_response(request("get", "/v1/suites/full_suite/f/tree?content=full&gen_vars=true"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.contains("f"));
        // Check family attributes
        BOOST_REQUIRE(content["f"].contains("attributes"));
        BOOST_REQUIRE(content["f"]["attributes"].size() == 4);
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
        BOOST_REQUIRE(content["f"]["children"]["t"]["attributes"].size() == 13);
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

    // Clean up
    request("delete", "/v1/suites/full_suite/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/full_suite/definition"); });
}

BOOST_AUTO_TEST_CASE(test_node_info) {
    ECF_NAME_THIS_TEST();

    // Clean up
    request("delete", "/v1/suites/suiteX/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/suiteX/definition"); });

    // Create suite 'suiteX'
    //       `-- suiteX
    //         `-- f
    //           `-- t
    handle_response(request("post",
                            "/v1/suites",
                            R"({"definition" : "suite suiteX\n  family f\n    task t\n  endfamily\nendsuite"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/suiteX/definition"); });

    // Check single-element array with correct path, "unknown" state, null time change
    //     This is done before beginning to ensure we handle the "unknown" state, and null time change correctly
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        const auto& entry = content[0];
        BOOST_REQUIRE(entry.contains("path"));
        BOOST_REQUIRE(entry["path"] == "/suiteX");
        BOOST_REQUIRE(entry.contains("state"));
        BOOST_REQUIRE(entry["state"] == "unknown");
        BOOST_REQUIRE(entry.contains("state_change_time"));
        BOOST_REQUIRE(entry["state_change_time"].is_null());
    }

    // Check single-element array with correct full path
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/f/t/info"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
        BOOST_REQUIRE(content[0]["state"] == "unknown");
    }

    // Check recursive=1, must include all nodes in the tree (suite + family + task = 3)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3); // suiteX, f, t

        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
        BOOST_REQUIRE(content[2]["path"] == "/suiteX/f/t");
    }

    // Check recursive=1 + type=task, must return only the task
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=task"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
    }

    // Check recursive=1 + type=family,task, must return family and task (2 nodes)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=family,task"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 2);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f/t");
    }

    // Check type=task without recursive on a suite node, must return empty array since suite is not a task
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?type=task"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.empty());
    }

    // Check type=task directly on the task node, must return the task itself
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/f/t/info?type=task"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
    }

    // Check type=suite on the suite node, must return the suite itself
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?type=suite"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
    }

    // Check that can handle invalid type value (invalid) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?type=invalid"), HttpStatusCode::client_error_bad_request);

    // Check that can handle invalid type value (uppercase type not accepted) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?type=Task"), HttpStatusCode::client_error_bad_request);

    // Check that a separator-only type value (no actual tokens) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?type=,"), HttpStatusCode::client_error_bad_request);
    handle_response(request("get", "/v1/suites/suiteX/info?type=,,"), HttpStatusCode::client_error_bad_request);

    // Check state=unknown, must return suite (n.b. no recursive)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
    }

    // Check recursive=1&state=unknown, must return all 3 nodes
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
    }

    // Check recursive=1&state=complete, must return empty array as all nodes are in unknown state before begin
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&state=complete"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.empty());
    }

    // Check state=queued,unknown, must consider both valid states (OR)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?state=queued,unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1); // suite is unknown, matches first term
    }

    // Check that can handle invalid state value (uppercase state name) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?state=Active"), HttpStatusCode::client_error_bad_request);

    // Check that can handle invalid state value (invalid state name) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?state=invalid"), HttpStatusCode::client_error_bad_request);

    // Check that a separator-only state value (no actual tokens) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?state=,"), HttpStatusCode::client_error_bad_request);
    handle_response(request("get", "/v1/suites/suiteX/info?state=,,"), HttpStatusCode::client_error_bad_request);

    // Check combined type+state (AND semantics): type=suite AND state=unknown
    //      must return exactly the suite root (1 result, non-recursive)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?type=suite&state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
    }

    // Check combined type+state: type=task AND state=unknown AND recursive=1
    //      must return only the task (not the suite or family)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=task&state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
    }

    // Check combined type+state: type=task AND state=complete AND recursive=1
    //      must return empty (no completed tasks before begin)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=task&state=complete"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.empty());
    }

    // Check combined type+state: type=suite,family AND state=unknown AND recursive=1
    //      must return suite + family (2 results), not the task
    {
        auto result =
            handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=suite,family&state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 2);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
    }

    // Check combined type+state: type=task AND state=queued AND recursive=1
    //      must return empty before begin (task is unknown, not queued)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&type=task&state=queued"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.empty());
    }

    // Begin the suite
    handle_response(request("put", "/v1/suites/suiteX/status", R"({"action":"begin"})", API_KEY));

    // Check that state_change_time is populated and state is no longer "unknown"
    wait_until([] {
        try {
            auto r       = handle_response(request("get", "/v1/suites/suiteX/info"), HttpStatusCode::success_ok, true);
            auto content = ojson::parse(r.body);
            return content.is_array() && !content[0]["state_change_time"].is_null() &&
                   content[0]["state"].get<std::string>() != "unknown";
        }
        catch (...) {
            return false;
        }
    });

    // Check that can handle non-existent node returns 404
    handle_response(request("get", "/v1/suites/suiteX/nonexistent/info"), HttpStatusCode::client_error_not_found);

    // Check that filter=[0] returns the first element as an object
    //      n.b. the filter query parameter is a generic functionality, applied to all endpoints
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?filter=[0]"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_object());
        BOOST_REQUIRE(content.contains("path"));
        BOOST_REQUIRE(content.contains("state"));
        BOOST_REQUIRE(content.contains("state_change_time"));
    }

    // Check sortby=+path, must return output in ascending lexicographic order of the path
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=%2Bpath"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
        BOOST_REQUIRE(content[2]["path"] == "/suiteX/f/t");
    }

    // Check sortby=-path, must return output in descending lexicographic order of the path
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=-path"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
        BOOST_REQUIRE(content[2]["path"] == "/suiteX");
    }

    // Check sortby=path, must return output (by default) in ascending lexicographic order of the path
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=path"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
        BOOST_REQUIRE(content[2]["path"] == "/suiteX/f/t");
    }

    // Check sortby=state, must return output (by default) in ascending alphabetical order of the state
    //
    // Note: The check verifies only that the order is correct, but not the values themselves because at this
    // point the suite is running and the states change as the suite progresses, so the exact values are not known.
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=state"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        for (size_t i = 1; i < content.size(); ++i) {
            BOOST_REQUIRE(content[i - 1]["state"].get<std::string>() <= content[i]["state"].get<std::string>());
        }
    }

    // Check sortby=-state, must return output in descending alphabetical order of the state
    //
    // Note: The check verifies only that the order is correct, but not the values themselves because at this
    // point the suite is running and the states change as the suite progresses, so the exact values are not known.
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=-state"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        for (size_t i = 1; i < content.size(); ++i) {
            BOOST_REQUIRE(content[i - 1]["state"].get<std::string>() >= content[i]["state"].get<std::string>());
        }
    }

    // Check sortby=state_change_time, must return output in ascending order of the time (nulls first, then ISO
    // strings)
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=state_change_time"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
        for (size_t i = 1; i < content.size(); ++i) {
            const bool a_null = content[i - 1]["state_change_time"].is_null();
            const bool b_null = content[i]["state_change_time"].is_null();
            if (!a_null && !b_null) {
                BOOST_REQUIRE(content[i - 1]["state_change_time"].get<std::string>() <=
                              content[i]["state_change_time"].get<std::string>());
            }
            else {
                BOOST_REQUIRE(!b_null || a_null); // null must not follow a non-null (null comes first)
            }
        }
    }

    // Check that can handle invalid sortby value → 400
    handle_response(request("get", "/v1/suites/suiteX/info?sortby=invalid_field"),
                    HttpStatusCode::client_error_bad_request);

    // Check count=2, must return only 2 items
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&count=2"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 2);
    }

    // Check count=0, must return empty array
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&count=0"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.empty());
    }

    // Check count=10, must return all 3 items, since N is larger than the result set
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&count=10"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
    }

    // Check that can handle invalid count values (negative values) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?count=-1"), HttpStatusCode::client_error_bad_request);

    // Check that can handle invalid count values (alphanumerical values) → 400
    handle_response(request("get", "/v1/suites/suiteX/info?count=abc1"), HttpStatusCode::client_error_bad_request);

    // Check sortby=-path&count=2, must sort by descending path, and then truncate output to 2
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteX/info?recursive=1&sortby=-path&count=2"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 2);
        BOOST_REQUIRE(content[0]["path"] == "/suiteX/f/t");
        BOOST_REQUIRE(content[1]["path"] == "/suiteX/f");
    }

    // Check HEAD requests: must return 200 with an empty body on both /info endpoints
    //       HEAD is declared as an allowed method in the OPTIONS handler; cpp-httplib automatically
    //       serves HEAD by running the GET handler and stripping the response body.
    {
        httplib::SSLClient c(API_HOST, ECF_TEST_HTTP_PORT_NR);
        c.enable_server_certificate_verification(false);
        c.set_connection_timeout(3);
        c.set_read_timeout(5);
        c.set_write_timeout(5);
        c.set_default_headers(httplib::Headers{{"Content-type", "application/json"}});

        // HEAD /v1/suites/suiteX/info
        auto r1 = c.Head("/v1/suites/suiteX/info");
        BOOST_REQUIRE(r1);
        BOOST_REQUIRE_EQUAL(r1->status, HttpStatusCode::success_ok);
        BOOST_REQUIRE(r1->body.empty());

        // HEAD /v1/suites/info
        auto r2 = c.Head("/v1/suites/info");
        BOOST_REQUIRE(r2);
        BOOST_REQUIRE_EQUAL(r2->status, HttpStatusCode::success_ok);
        BOOST_REQUIRE(r2->body.empty());
    }

    // Clean up
    request("delete", "/v1/suites/suiteX/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/suiteX/definition"); });
}

BOOST_AUTO_TEST_CASE(test_suites_info) {
    ECF_NAME_THIS_TEST();

    // Clean up
    //       |-- suite1
    //       | `-- fa
    //       |   `-- ta
    //       `-- suite2
    //         `-- fb
    //           `-- tb
    request("delete", "/v1/suites/suite1/definition", "", API_KEY);
    request("delete", "/v1/suites/suite2/definition", "", API_KEY);
    wait_until([] {
        return !check_for_path("/v1/suites/suite1/definition") && !check_for_path("/v1/suites/suite2/definition");
    });

    // Create two suites: suite1 (family fa, task ta) and suite2 (family fb, task tb)
    handle_response(request("post",
                            "/v1/suites",
                            R"({"definition" : "suite suite1\n  family fa\n    task ta\n  endfamily\nendsuite"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    handle_response(request("post",
                            "/v1/suites",
                            R"({"definition" : "suite suite2\n  family fb\n    task tb\n  endfamily\nendsuite"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] {
        return check_for_path("/v1/suites/suite1/definition") && check_for_path("/v1/suites/suite2/definition");
    });

    // Check without recursive, must return both suite roots
    {
        auto result  = handle_response(request("get", "/v1/suites/info"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() >= 2);
        bool has_a = false, has_b = false;
        for (const auto& entry : content) {
            if (entry["path"] == "/suite1") {
                has_a = true;
            }
            if (entry["path"] == "/suite2") {
                has_b = true;
            }
        }
        BOOST_REQUIRE(has_a);
        BOOST_REQUIRE(has_b);
    }

    // Check with recursive=1: must return suite1 and suite2 (each with 3 nodes)
    {
        auto result  = handle_response(request("get", "/v1/suites/info?recursive=1"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        size_t count_a = 0, count_b = 0;
        for (const auto& entry : content) {
            const auto path = entry["path"].get<std::string>();
            if (path == "/suite1" || path.rfind("/suite1/", 0) == 0) {
                ++count_a;
            }
            if (path == "/suite2" || path.rfind("/suite2/", 0) == 0) {
                ++count_b;
            }
        }
        BOOST_REQUIRE(count_a == 3); // suite + family + task
        BOOST_REQUIRE(count_b == 3); // suite + family + task
    }

    // Check recursive=1 + type=task, must return 2 tasks, one from each suite
    {
        auto result  = handle_response(request("get", "/v1/suites/info?recursive=1&type=task"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        bool has_ta = false, has_tb = false;
        for (const auto& entry : content) {
            const auto path = entry["path"].get<std::string>();
            if (path == "/suite1/fa/ta") {
                has_ta = true;
                BOOST_REQUIRE(entry["state"] == "unknown");
            }
            if (path == "/suite2/fb/tb") {
                has_tb = true;
                BOOST_REQUIRE(entry["state"] == "unknown");
            }
        }
        BOOST_REQUIRE(has_ta);
        BOOST_REQUIRE(has_tb);
    }

    // Check sortby=-path with recursive=1, must return entries in descending path order
    {
        auto result  = handle_response(request("get", "/v1/suites/info?recursive=1&sortby=-path"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() >= 2);
        std::string first = content[0]["path"].get<std::string>();
        std::string last  = content[content.size() - 1]["path"].get<std::string>();
        BOOST_REQUIRE(first > last);
    }

    // Check count=3, must truncate the result to exactly 3 entries
    {
        auto result  = handle_response(request("get", "/v1/suites/info?recursive=1&count=3"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 3);
    }

    // Check state=unknown without recursive, must return both suite1 and suite2
    {
        auto result  = handle_response(request("get", "/v1/suites/info?state=unknown"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        bool has_a = false, has_b = false;
        for (const auto& entry : content) {
            if (entry["path"] == "/suite1") {
                has_a = true;
            }
            if (entry["path"] == "/suite2") {
                has_b = true;
            }
        }
        BOOST_REQUIRE(has_a);
        BOOST_REQUIRE(has_b);
    }

    // Check state=active without recursive, must return an empty array since neither suite is active
    {
        auto result  = handle_response(request("get", "/v1/suites/info?state=active"));
        auto content = ojson::parse(result.body);
        BOOST_REQUIRE(content.is_array());
        for (const auto& entry : content) {
            // Neither of our test suites should appear as active
            BOOST_REQUIRE(entry["path"] != "/suite1");
            BOOST_REQUIRE(entry["path"] != "/suite2");
        }
    }

    // Clean up
    request("delete", "/v1/suites/suite1/definition", "", API_KEY);
    request("delete", "/v1/suites/suite2/definition", "", API_KEY);
    wait_until([] {
        return !check_for_path("/v1/suites/suite1/definition") && !check_for_path("/v1/suites/suite2/definition");
    });
}

BOOST_AUTO_TEST_CASE(test_node_info_aliases) {
    ECF_NAME_THIS_TEST();

    // Aliases are 'special' children of a Task.
    //
    // This test ensures that the traversal of the .../info endpoint reaches aliases, when using 'type=alias' filter

    // Clean up
    request("delete", "/v1/suites/suiteY/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/suiteY/definition"); });

    // Create suite 'suiteY'
    //       `-- suiteY
    //         `-- f
    //           `-- t
    handle_response(request("post",
                            "/v1/suites",
                            R"({"definition" : "suite suiteY\n  family f\n    task t\n  endfamily\nendsuite"})",
                            API_KEY),
                    HttpStatusCode::success_created);
    wait_until([] { return check_for_path("/v1/suites/suiteY/definition"); });

    // Create an alias under the task '/suiteY/f/t' directly on the ecFlow server.
    //     Note: we use ecFlow Python API to create aliases.
    {
        std::string port = ecf::environment::get("ECF_PORT");
        ClientInvoker client("localhost", port);
#if defined(ECF_TEST_HTTP_BACKEND)
        client.enable_http();
#endif
        std::vector<std::string> user_file_contents = {"%comment", "%end", "echo \"alias body\""};
        BOOST_REQUIRE_NO_THROW(client.edit_script_submit(
            "/suiteY/f/t", NameValueVec{}, user_file_contents, true /* create_alias */, false /* run */));
    }

    // Wait for the REST server to pick up the newly created alias '/suiteY/f/t/alias0'
    wait_until([] {
        try {
            auto r = handle_response(
                request("get", "/v1/suites/suiteY/info?recursive=1&type=alias"), HttpStatusCode::success_ok, true);
            auto content = ojson::parse(r.body);
            return content.is_array() && !content.empty() && content[0]["path"] == "/suiteY/f/t/alias0";
        }
        catch (...) {
            return false;
        }
    });

    // Check recursive=1, must return 4 nodes, including suite + family + task + alias
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteY/info?recursive=1"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        bool has_alias = false;
        for (const auto& entry : content) {
            if (entry["path"] == "/suiteY/f/t/alias0") {
                has_alias = true;
            }
        }
        BOOST_REQUIRE(has_alias);
    }

    // Check recursive=1 + type=alias, must return exactly only the alias
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteY/info?recursive=1&type=alias"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteY/f/t/alias0");
    }

    // Check recursive=1 + type=task,alias, must return both the task and its alias
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteY/info?recursive=1&type=task,alias"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 2);
        bool has_task = false, has_alias = false;
        for (const auto& entry : content) {
            if (entry["path"] == "/suiteY/f/t") {
                has_task = true;
            }
            if (entry["path"] == "/suiteY/f/t/alias0") {
                has_alias = true;
            }
        }
        BOOST_REQUIRE(has_task);
        BOOST_REQUIRE(has_alias);
    }

    // Check that querying the alias node directly (without recursive) returns the alias itself
    {
        auto result  = handle_response(request("get", "/v1/suites/suiteY/f/t/alias0/info"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        BOOST_REQUIRE(content.size() == 1);
        BOOST_REQUIRE(content[0]["path"] == "/suiteY/f/t/alias0");
    }

    // Check that aliases are accessible through /v1/suites/info endpoint
    {
        auto result  = handle_response(request("get", "/v1/suites/info?recursive=1&type=alias"));
        auto content = ojson::parse(result.body);

        BOOST_REQUIRE(content.is_array());
        bool has_alias = false;
        for (const auto& entry : content) {
            if (entry["path"] == "/suiteY/f/t/alias0") {
                has_alias = true;
            }
        }
        BOOST_REQUIRE(has_alias);
    }

    // Clean up
    request("delete", "/v1/suites/suiteY/definition", "", API_KEY);
    wait_until([] { return false == check_for_path("/v1/suites/suiteY/definition"); });
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

    handle_response(
        request("post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", API_KEY_pbkdf2),
        HttpStatusCode::success_created);
    wait_until([] { return check_for_element("/v1/server/attributes?filter=variables", "value", "xfoo", "xbar"); });

    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", API_KEY_expired),
        HttpStatusCode::client_error_unauthorized);

    handle_response(
        request(
            "post", "/v1/server/attributes", R"({"type":"variable","name":"xfoo","value":"xbar"})", API_KEY_revoked),
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
