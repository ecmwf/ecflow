/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/ApiV1.hpp"

#include <atomic>
#include <mutex>
#include <string>

#include <sys/time.h>

#include "ecflow/core/Str.hpp"
#include "ecflow/http/ApiV1Impl.hpp"
#include "ecflow/http/Client.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/Options.hpp"
#include "ecflow/http/TypeToJson.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf::http {

static std::chrono::system_clock::time_point api_startup;

std::atomic<unsigned int> num_requests(0);
std::atomic<unsigned int> num_errors(0);
std::atomic<unsigned int> num_cached_requests(0);
std::atomic<unsigned int> last_request_time(0);

namespace /* __anonymous__ */ {

void set_cors(httplib::Response& response) {
    response.set_header("Access-Control-Allow-Origin", "*");
    response.set_header("Access-Control-Allow-Credentials", "true");
    response.set_header("Access-Control-Allow-Headers", "*");
}

void set_allowed_methods(httplib::Response& response, const std::string& methods) {
    response.set_header("Allow", methods);
    response.set_header("Access-Control-Allow-Methods", methods);
}

void handle_exception(const std::exception& e, const httplib::Request& request, httplib::Response& response) {
    // Try to guess a bit more suitable return values
    // based on client output
    // TODO: should this actually be done? eg. do the status codes *only*
    // reflect the API functionality or also the application (ecflow) server?
    // if user requests an ecflow script and the script does not exists, should
    // 404 be returned (even if the rest api path was correct)?

    auto trimr = [](const std::string& str) {
        std::string copy = str;
        copy.erase(copy.find_last_not_of(" \n\r") + 1, std::string::npos);
        return copy;
    };

    HttpStatusCode status_code = HttpStatusCode::client_error_bad_request;

    const std::string err = trimr(std::string(e.what()));

    if (err.find("authentication failed") != std::string::npos) {
        status_code = HttpStatusCode::client_error_unauthorized;
    }
    else if (err.find("Could not find") != std::string::npos) {
        status_code = HttpStatusCode::client_error_not_found;
    }
    else if (err.find("Cannot find") != std::string::npos) {
        status_code = HttpStatusCode::client_error_not_found;
    }
    else if (err.find("cannot be found") != std::string::npos &&
             err.find("Could not find referenced node") == std::string::npos) {
        status_code = HttpStatusCode::client_error_not_found;
    }
    else if (err.find("No such file") != std::string::npos) {
        status_code = HttpStatusCode::client_error_not_found;
    }
    else if (err.find("Add Suite failed: A Suite of name") != std::string::npos) {
        status_code = HttpStatusCode::client_error_conflict;
    }
    else if (err.find("Ran out of end points") != std::string::npos) {
        status_code = HttpStatusCode::server_error_bad_gateway;
    }
    else if (err.find("Failed to connect to ") != std::string::npos) {
        status_code = HttpStatusCode::server_error_bad_gateway;
    }
    else if (err.find("Cannot replace node ") != std::string::npos) {
        status_code = HttpStatusCode::server_error_bad_gateway;
    }

    ojson j;
    j["code"]    = status_code;
    j["message"] = err;
    j["path"]    = request.path;
    j["method"]  = request.method;
    if (request.body.empty() == false) {
        j["body"] = request.body;
    }
    response.set_content(j.dump(), "application/json");
    response.status = status_code;
    set_cors(response);
}

void handle_exception(const HttpServerException& e, const httplib::Request& request, httplib::Response& response) {
    ojson j;
    j["code"]    = e.code();
    j["message"] = e.what();
    j["path"]    = request.path;
    j["method"]  = request.method;
    if (request.body.empty() == false) {
        j["body"] = request.body;
    }

    response.set_content(j.dump(), "application/json");
    response.status = e.code();
    set_cors(response);
}

void set_last_request_time() {
    struct timeval curtime
    {
    };
    gettimeofday(&curtime, nullptr);
    last_request_time = static_cast<unsigned int>(curtime.tv_sec);
}

template <typename T>
void trycatch(const httplib::Request& request, httplib::Response& response, T&& func) {
    set_last_request_time();
    try {
        num_requests++;
        func();
    }
    catch (const HttpServerException& e) {
        num_errors++;
        handle_exception(e, request, response);
    }
    catch (const std::exception& e) {
        num_errors++;
        handle_exception(e, request, response);
    }
}

ojson filter_json(const ojson& j, const httplib::Request& r) {

    const std::string path = r.get_param_value("filter");
    if (path.empty()) {
        return j;
    }

    // split filter path on dot, and reverse the elements
    // the elements are consumed by the dive() function

    std::vector<std::string> path_elems;
    ecf::Str::split(path, path_elems, ".");
    std::reverse(path_elems.begin(), path_elems.end());

    if (path_elems.empty()) {
        return j;
    }

    // separate array name and index from a string
    auto get_array_info = [](std::string_view str) {
        const auto start = str.find("[");
        const auto stop  = str.find("]");
        const auto key   = str.substr(0, start);
        const int index  = std::stoi(std::string{str.substr(start + 1, stop - start)});
        return std::make_pair(key, index);
    };

    // special case: filter is .[INDEX], means that we return the
    // correct array element from root json element assuming it's an array
    if (path_elems.size() == 1 && path_elems[0][0] == '[' && path_elems[0][path_elems[0].size() - 1] == ']') {
        const auto [key, index] = get_array_info(path_elems[0]);
        (void)key; // Note: suppress unused variable warning
        return j[index];
    }

    // recursively find the correct element inside json document
    std::function<ojson(const ojson&, std::vector<std::string>&)> dive = [&](const ojson& js,
                                                                             const std::vector<std::string>& elems) {
        if (elems.empty()) {
            return js;
        }

        const auto elem = path_elems.back();
        path_elems.pop_back();

        try {
            if (elem.find("[") != std::string::npos && elem.find("]") != std::string::npos) {
                const auto [key, index] = get_array_info(elem);
                return dive(js.at(std::string{key})[index], path_elems);
            }

            return dive(js.at(elem), path_elems);
        }
        catch (const ojson::exception& e [[maybe_unused]]) {
            // filter path is not found or some other problem with user given path
            return ojson();
        }
    };

    return dive(j, path_elems);
}

static std::string get_tree_content_kind(const httplib::Request& request) {
    constexpr const char* parameter = "content";
    return request.has_param(parameter) ? request.get_param_value(parameter) : "basic";
}

static bool get_tree_content_id_flag(const httplib::Request& request) {
    constexpr const char* parameter = "with_id";
    return request.has_param(parameter) ? (request.get_param_value(parameter) == "true" ? true : false) : false;
}

static bool get_tree_content_gen_vars(const httplib::Request& request) {
    constexpr const char* parameter = "gen_vars";
    return request.has_param(parameter) ? (request.get_param_value(parameter) == "true" ? true : false) : false;
}

} // namespace

namespace v1 {

void not_implemented(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::server_error_not_implemented;
        set_cors(response);
    });
}

void suites_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, POST, HEAD");
        set_cors(response);
    });
}

void suites_create(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        add_suite(request, response);
        set_cors(response);
    });
}

void suites_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        ojson j = filter_json(ecf::http::get_suites(), request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_tree_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, HEAD");
        set_cors(response);
    });
}

void node_tree_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        const std::string path = request.matches[1];
        std::string tree_kind  = get_tree_content_kind(request);
        bool with_id           = get_tree_content_id_flag(request);
        bool gen_vars          = get_tree_content_gen_vars(request);
        ojson tree_content     = (tree_kind == "full") ? get_full_node_tree(path, with_id, gen_vars) : get_basic_node_tree(path);
        ojson j                = filter_json(tree_content, request);
        response.status        = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_definition_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, PUT, DELETE, HEAD");
        set_cors(response);
    });
}

void node_definition_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        const std::string path = request.matches[1];
        ojson j                = filter_json(get_node_definition(path), request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_definition_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j = update_node_definition(request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/ojson");
        set_cors(response);
    });
}

void node_definition_delete(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        const std::string path = request.matches[1];
        auto client            = get_client(request);
        client->delete_node(path);

        ojson j;
        j["message"]    = "Node deleted successfully";
        response.status = HttpStatusCode::success_no_content;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_status_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, PUT, HEAD");
        set_cors(response);
    });
}

void node_status_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        ojson j = filter_json(get_node_status(request), request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_status_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = update_node_status(request);
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_attributes_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, PUT, POST, DELETE, HEAD");
        set_cors(response);
    });
}

void node_attributes_create(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = add_node_attribute(request);
        response.status = HttpStatusCode::success_created;
        response.set_content(j.dump(), "application/ojson");
        response.set_header("Location", "/v1/suites" + static_cast<std::string>(request.matches[1]) + "/attributes");
        set_cors(response);
    });
}

void node_attributes_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        const std::string path = request.matches[1];
        ojson j                = filter_json(get_node_attributes(path), request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_attributes_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = update_node_attribute(request);
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_attributes_delete(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = delete_node_attribute(request);
        response.status = HttpStatusCode::success_no_content;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_script_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, HEAD");
        set_cors(response);
    });
}

void node_script_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        const std::string path = request.matches[1];
        auto client            = get_client(request);

        ojson j;

        client->file(path, "script");
        j["script"] = client->server_reply().get_string();

        try {
            client->file(path, "job");
            j["job"] = client->server_reply().get_string();
        }
        catch (const std::exception& e [[maybe_unused]]) {
            j["job"] = "";
        }

        j               = filter_json(j, request);
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_script_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = update_script_content(request);
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void node_output_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, HEAD");
        set_cors(response);
    });
}

void node_output_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j         = filter_json(get_node_output(request), request);
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_ping_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, HEAD");
        set_cors(response);
    });
}

void server_ping_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        auto client = get_client(request);
        client->pingServer();

        ojson j;
        j["host"]            = client->host() + ":" + client->port();
        j["round_trip_time"] = to_simple_string(client->round_trip_time());
        j                    = filter_json(j, request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_status_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, PUT, HEAD");
        set_cors(response);
    });
}

void server_status_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        auto client = get_client(request);

        client->stats_server();
        ojson j = client->server_reply().stats();
        j       = filter_json(j, request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_status_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        auto payload = ojson::parse(request.body);
        if (const std::string name = payload.at("action"); name == "reload_whitelist_file") {
            get_client(request)->reloadwsfile();
        }
        else if (name == "reload_passwd_file") {
            get_client(request)->reloadpasswdfile();
        }
        else if (name == "reload_custom_passwd_file") {
            get_client(request)->reloadcustompasswdfile();
        }
        else {
            throw HttpServerException(HttpStatusCode::client_error_bad_request, "Invalid action: " + name);
        }

        ojson j;
        j["message"]    = "Server updated successfully";
        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_attributes_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, PUT, POST, DELETE, HEAD");
        set_cors(response);
    });
}

void server_attributes_create(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j = add_server_attribute(request);

        response.status = HttpStatusCode::success_created;
        response.set_content(j.dump(), "application/json");
        response.set_header("Location", "/v1/server/attributes");
        set_cors(response);
    });
}

void server_attributes_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        num_cached_requests++;
        ojson j = filter_json(get_server_attributes(), request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_attributes_update(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j = update_server_attribute(request);

        response.status = HttpStatusCode::success_ok;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_attributes_delete(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        ojson j = delete_server_attribute(request);

        response.status = HttpStatusCode::success_no_content;
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

void server_statistics_options(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status = HttpStatusCode::success_no_content;
        set_allowed_methods(response, "GET, HEAD");
        set_cors(response);
    });
}

void server_statistics_read(const httplib::Request& request, httplib::Response& response) {
    trycatch(request, response, [&]() {
        response.status     = HttpStatusCode::success_ok;
        const std::time_t t = std::chrono::system_clock::to_time_t(api_startup);
        std::array<char, 80> date{};
        std::tm tm{};
        gmtime_r(&t, &tm);
        strftime(date.data(), 80, "%Y-%m-%dT%H:%M:%SZ", &tm);

        ojson j = {{"num_requests", num_requests.load()},
                   {"num_errors", num_errors.load()},
                   {"num_cached_requests", num_cached_requests.load()},
                   {"since", std::string{date.data()}}};

        j = filter_json(j, request);
        response.set_content(j.dump(), "application/json");
        set_cors(response);
    });
}

} // namespace v1

void routing(httplib::Server& http_server) {
    if (opts.verbose) {
        printf("Registering API location /v1\n");
    }

    /* .../suites */
    {
        std::string resource_path = R"(/v1/suites)";

        http_server.Options(resource_path, v1::suites_options); // Options
        http_server.Post(resource_path, v1::suites_create);     // Create
        http_server.Get(resource_path, v1::suites_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);    // Update
        http_server.Delete(resource_path, v1::not_implemented); // Delete
    }

    /* .../suites/<path>/tree */
    {
        std::string resource_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/tree)";
        std::string resource_root = R"(/v1/suites(/)tree)";

        http_server.Options(resource_path, v1::node_tree_options); // Options
        http_server.Post(resource_path, v1::not_implemented);      // Create
        http_server.Get(resource_root, v1::node_tree_read);        // Read
        http_server.Get(resource_path, v1::node_tree_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);       // Updat
        http_server.Delete(resource_path, v1::not_implemented);    // Delete
    }

    /* .../suites/<path>/definition */
    {
        std::string resourse_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/definition$)";

        http_server.Options(resourse_path, v1::node_definition_options); // Options
        http_server.Post(resourse_path, v1::not_implemented);            // Create
        http_server.Get(resourse_path, v1::node_definition_read);        // Read
        http_server.Put(resourse_path, v1::node_definition_update);      // Update
        http_server.Delete(resourse_path, v1::node_definition_delete);   // Delete
    }

    /* .../suites/<path>/status */
    {
        std::string resource_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/status$)";

        http_server.Options(resource_path, v1::node_status_options); // Options
        http_server.Post(resource_path, v1::not_implemented);        // Create
        http_server.Get(resource_path, v1::node_status_read);        // Read
        http_server.Put(resource_path, v1::node_status_update);      // Update
        http_server.Delete(resource_path, v1::not_implemented);      // Delete
    }

    /* .../suites/<path>/attributes */
    {
        std::string resource_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)";

        http_server.Options(resource_path, v1::node_attributes_options); // Options
        http_server.Post(resource_path, v1::node_attributes_create);     // Create
        http_server.Get(resource_path, v1::node_attributes_read);        // Read
        http_server.Put(resource_path, v1::node_attributes_update);      // Update
        http_server.Delete(resource_path, v1::node_attributes_delete);   // Delete
    }

    /* .../suites/<path>/script */
    {
        std::string resource_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/script$)";

        http_server.Options(resource_path, v1::node_script_options); // Options
        http_server.Post(resource_path, v1::not_implemented);        // Create
        http_server.Get(resource_path, v1::node_script_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);         // Update
        http_server.Delete(resource_path, v1::not_implemented);      // Delete
    }

    /* .../suites/<path>/output */
    {
        std::string resource_path = R"(/v1/suites([A-Za-z0-9_\/\.]+)/output$)";

        http_server.Options(resource_path, v1::node_output_options); // Options
        http_server.Post(resource_path, v1::not_implemented);        // Create
        http_server.Get(resource_path, v1::node_output_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);         // Update
        http_server.Delete(resource_path, v1::not_implemented);      // Delete
    }

    /* .../server/ping */
    {
        std::string resource_path = R"(/v1/server/ping)";

        http_server.Options(resource_path, v1::server_ping_options); // Options
        http_server.Post(resource_path, v1::not_implemented);        // Create
        http_server.Get(resource_path, v1::server_ping_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);         // Update
        http_server.Delete(resource_path, v1::not_implemented);      // Delete
    }

    /* .../server/status */
    {
        std::string resource_path = R"(/v1/server/status)";

        http_server.Options("/v1/server/status", v1::server_status_options); // Options
        http_server.Post(resource_path, v1::not_implemented);                // Create
        http_server.Get(resource_path, v1::server_status_read);              // Read
        http_server.Put("/v1/server/status", v1::server_status_update);      // Update
        http_server.Delete(resource_path, v1::not_implemented);              // Delete
    }

    /* .../server/attributes */
    {
        std::string resource_path = R"(/v1/server/attributes)";

        http_server.Options(resource_path, v1::server_attributes_options); // Options
        http_server.Post(resource_path, v1::server_attributes_create);     // Create
        http_server.Get(resource_path, v1::server_attributes_read);        // Read
        http_server.Put(resource_path, v1::server_attributes_update);      // Update
        http_server.Delete(resource_path, v1::server_attributes_delete);   // Delete
    }

    /* .../statistics */
    {
        std::string resource_path = R"(/v1/statistics)";

        http_server.Options(resource_path, v1::server_statistics_options); // Options
        http_server.Post(resource_path, v1::not_implemented);              // Create
        http_server.Get(resource_path, v1::server_statistics_read);        // Read
        http_server.Put(resource_path, v1::not_implemented);               // Update
        http_server.Delete(resource_path, v1::not_implemented);            // Delete
    }

    api_startup = std::chrono::system_clock::now();
    set_last_request_time();
}

} // namespace ecf::http
