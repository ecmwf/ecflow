/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ApiV1.hpp"

#include <atomic>
#include <mutex>
#include <string>

#include <sys/time.h>

#include "ApiV1Impl.hpp"
#include "Defs.hpp"
#include "HttpServerException.hpp"
#include "Options.hpp"
#include "Str.hpp"
#include "TypeToJson.hpp"

extern Options opts;
static std::chrono::system_clock::time_point api_startup;
std::atomic<unsigned int> num_requests(0);
std::atomic<unsigned int> num_errors(0);
std::atomic<unsigned int> num_cached_requests(0);
std::atomic<unsigned int> last_request_time(0);

namespace {

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

    auto trimr = [](const std::string& str) -> std::string {
        std::string copy = str;
        copy.erase(copy.find_last_not_of(" \n\r") + 1, std::string::npos);
        return copy;
    };

    HttpStatusCode status_code = HttpStatusCode::client_error_bad_request;

    const std::string err      = trimr(std::string(e.what()));

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

    ecf::ojson j;
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
    ecf::ojson j;
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
    struct timeval curtime;
    gettimeofday(&curtime, nullptr);
    last_request_time = curtime.tv_sec;
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

ecf::ojson filter_json(const ecf::ojson& j, const httplib::Request& r) {

    const std::string path = r.get_param_value("filter");
    if (path.empty())
        return j;

    // split filter path on dot, and reverse the elements
    // the elements are consumed by the dive() function

    std::vector<std::string> path_elems;
    ecf::Str::split(path, path_elems, ".");
    std::reverse(path_elems.begin(), path_elems.end());

    if (path_elems.empty())
        return j;

    // separate array name and index from a string
    auto get_array_info = [](const std::string& str) {
        auto start = str.find("["), stop = str.find("]");
        const std::string key = str.substr(0, start);
        const int index       = std::stoi(str.substr(start + 1, stop - start));
        return std::make_pair(key, index);
    };

    // special case: filter is .[INDEX], means that we return the
    // correct array element from root json element assuming it's an array
    if (path_elems.size() == 1 && path_elems[0][0] == '[' && path_elems[0][path_elems[0].size() - 1] == ']') {
        const auto arr = get_array_info(path_elems[0]);
        return j[arr.second];
    }

    // recursively find the correct element inside json document
    std::function<ecf::ojson(const ecf::ojson&, std::vector<std::string>&)> dive =
        [&](const ecf::ojson& js, std::vector<std::string>& path_elems) -> ecf::ojson {
        if (path_elems.empty())
            return js;

        const auto elem = path_elems.back();
        path_elems.pop_back();

        try {
            if (elem.find("[") != std::string::npos && elem.find("]") != std::string::npos) {
                const auto arr = get_array_info(elem);
                return dive(js.at(arr.first)[arr.second], path_elems);
            }

            return dive(js.at(elem), path_elems);
        }
        catch (const ecf::ojson::exception& e) {
            // filter path is not found or some other problem with user given path
            return ecf::ojson();
        }
    };

    return dive(j, path_elems);
}

void create(httplib::Server& http_server) {
    if (opts.verbose)
        printf("Registering API location /v1\n");

    http_server.Get("/v1/suites", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            num_cached_requests++;
            ecf::ojson j    = filter_json(get_suites(), request);

            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Post(R"(/v1/suites$)", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            add_suite(request, response);
            set_cors(response);
        });
    });

    http_server.Options("/v1/suites", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status = HttpStatusCode::success_no_content;
            set_allowed_methods(response, "GET, POST, HEAD");
            set_cors(response);
        });
    });

    /* ../tree */

    http_server.Get("/v1/suites/tree", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            num_cached_requests++;
            ecf::ojson j    = filter_json(get_sparser_node_tree("/"), request);
            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/tree$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/tree$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            num_cached_requests++;
                            const std::string path = request.matches[1];
                            ecf::ojson j           = filter_json(get_sparser_node_tree(path), request);
                            response.status        = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    /* ../definition */

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/definition$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, PUT, DELETE, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Delete(R"(/v1/suites([A-Za-z0-9_\/\.]+)/definition$)",
                       [](const httplib::Request& request, httplib::Response& response) {
                           trycatch(request, response, [&]() {
                               const std::string path = request.matches[1];
                               auto client            = get_client(request);
                               client->delete_node(path);

                               ecf::ojson j;
                               j["message"]    = "Node deleted successfully";
                               response.status = HttpStatusCode::success_no_content;
                               response.set_content(j.dump(), "application/json");
                               set_cors(response);
                           });
                       });

    http_server.Put(R"(/v1/suites([A-Za-z0-9_\/\.]+)/definition$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            ecf::ojson j    = update_node_definition(request);

                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/ecf::ojson");
                            set_cors(response);
                        });
                    });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/definition$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            num_cached_requests++;
                            const std::string path = request.matches[1];
                            ecf::ojson j           = filter_json(get_node_definition(path), request);

                            response.status        = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    /* .../status */

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/status$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, PUT, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/status$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            num_cached_requests++;
                            ecf::ojson j    = filter_json(get_node_status(request), request);

                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    http_server.Put(R"(/v1/suites([A-Za-z0-9_\/\.]+)/status$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            ecf::ojson j    = update_node_status(request);
                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    /* .../attributes */

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, PUT, POST, DELETE, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            num_cached_requests++;
                            const std::string path = request.matches[1];
                            ecf::ojson j           = filter_json(get_node_attributes(path), request);

                            response.status        = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    http_server.Post(R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)",
                     [](const httplib::Request& request, httplib::Response& response) {
                         trycatch(request, response, [&]() {
                             ecf::ojson j    = add_node_attribute(request);
                             response.status = HttpStatusCode::success_created;
                             response.set_content(j.dump(), "application/ecf::ojson");
                             response.set_header("Location",
                                                 "/v1/suites" + static_cast<std::string>(request.matches[1]) +
                                                     "/attributes");
                             set_cors(response);
                         });
                     });

    http_server.Put(R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            ecf::ojson j    = update_node_attribute(request);
                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    http_server.Delete(R"(/v1/suites([A-Za-z0-9_\/\.]+)/attributes$)",
                       [](const httplib::Request& request, httplib::Response& response) {
                           trycatch(request, response, [&]() {
                               ecf::ojson j    = delete_node_attribute(request);
                               response.status = HttpStatusCode::success_no_content;
                               response.set_content(j.dump(), "application/json");
                               set_cors(response);
                           });
                       });

    /* .../script */

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/script$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/script$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            const std::string path = request.matches[1];
                            auto client            = get_client(request);

                            ecf::ojson j;

                            client->file(path, "script");
                            j["script"] = client->server_reply().get_string();

                            try {
                                client->file(path, "job");
                                j["job"] = client->server_reply().get_string();
                            }
                            catch (const std::exception& e) {
                                j["job"] = "";
                            }

                            j               = filter_json(j, request);
                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });
#if 0
   http_server.Put(R"(/v1/suites([A-Za-z0-9_\/\.]+)/script$)",
                   [](const httplib::Request& request, httplib::Response& response) {
                      trycatch(request, response, [&]() {
                         ecf::ojson j = update_script_content(request);
                         response.status = HttpStatusCode::success_ok;
                         response.set_content(j.dump(), "application/json");
                         set_cors(response);
                      });
                   });
#endif

    /* ../output */

    http_server.Options(R"(/v1/suites([A-Za-z0-9_\/\.]+)/output$)",
                        [](const httplib::Request& request, httplib::Response& response) {
                            trycatch(request, response, [&]() {
                                response.status = HttpStatusCode::success_no_content;
                                set_allowed_methods(response, "GET, HEAD");
                                set_cors(response);
                            });
                        });

    http_server.Get(R"(/v1/suites([A-Za-z0-9_\/\.]+)/output$)",
                    [](const httplib::Request& request, httplib::Response& response) {
                        trycatch(request, response, [&]() {
                            ecf::ojson j    = filter_json(get_node_output(request), request);
                            response.status = HttpStatusCode::success_ok;
                            response.set_content(j.dump(), "application/json");
                            set_cors(response);
                        });
                    });

    /* /server */

    http_server.Get("/v1/server/ping", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            auto client = get_client(request);
            client->pingServer();

            ecf::ojson j;
            j["host"]            = client->host() + ":" + client->port();
            j["round_trip_time"] = to_simple_string(client->round_trip_time());
            j                    = filter_json(j, request);

            response.status      = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Options("/v1/server/ping", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status = HttpStatusCode::success_no_content;
            set_allowed_methods(response, "GET, HEAD");
            set_cors(response);
        });
    });

    http_server.Get("/v1/server/status", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            auto client = get_client(request);

            client->stats_server();
            ecf::ojson j    = client->server_reply().stats();
            j               = filter_json(j, request);

            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Put("/v1/server/status", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            const ecf::ojson payload = ecf::ojson::parse(request.body);
            const std::string name   = payload.at("action");

            auto client              = get_client(request);

            if (name == "reload_whitelist_file") {
                client->reloadwsfile();
            }
            else if (name == "reload_passwd_file") {
                client->reloadpasswdfile();
            }
            else if (name == "reload_custom_passwd_file") {
                client->reloadcustompasswdfile();
            }
            else {
                throw HttpServerException(HttpStatusCode::client_error_bad_request, "Invalid action: " + name);
            }

            ecf::ojson j;
            j["message"]    = "Server updated successfully";

            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Options("/v1/server/status", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status = HttpStatusCode::success_no_content;
            set_allowed_methods(response, "GET, PUT, HEAD");
            set_cors(response);
        });
    });

    http_server.Get("/v1/server/attributes", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            num_cached_requests++;
            ecf::ojson j    = filter_json(get_server_attributes(), request);

            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Post("/v1/server/attributes", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            ecf::ojson j    = add_server_attribute(request);

            response.status = HttpStatusCode::success_created;
            response.set_content(j.dump(), "application/json");
            response.set_header("Location", "/v1/server/attributes");
            set_cors(response);
        });
    });

    http_server.Put("/v1/server/attributes", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            ecf::ojson j    = update_server_attribute(request);

            response.status = HttpStatusCode::success_ok;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Delete("/v1/server/attributes", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            ecf::ojson j    = delete_server_attribute(request);

            response.status = HttpStatusCode::success_no_content;
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Options("/v1/server/attributes", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status = HttpStatusCode::success_no_content;
            set_allowed_methods(response, "GET, PUT, POST, DELETE, HEAD");
            set_cors(response);
        });
    });

    http_server.Get("/v1/statistics", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status     = HttpStatusCode::success_ok;
            const std::time_t t = std::chrono::system_clock::to_time_t(api_startup);
            char date[80];
            const std::tm tm = *gmtime(&t);
            strftime(date, 80, "%Y-%m-%dT%H:%M:%SZ", &tm);

            ecf::ojson j = {{"num_requests", num_requests.load()},
                            {"num_errors", num_errors.load()},
                            {"num_cached_requests", num_cached_requests.load()},
                            {"since", std::string(date)}};

            j            = filter_json(j, request);
            response.set_content(j.dump(), "application/json");
            set_cors(response);
        });
    });

    http_server.Options("/v1/statistics", [](const httplib::Request& request, httplib::Response& response) {
        trycatch(request, response, [&]() {
            response.status = HttpStatusCode::success_no_content;
            set_allowed_methods(response, "GET, HEAD");
            set_cors(response);
        });
    });

    api_startup = std::chrono::system_clock::now();
    set_last_request_time();
}

} // namespace

void ApiV1::create(httplib::Server& http_server) {
    ::create(http_server);
    update_defs_loop(opts.polling_interval);

    if (opts.verbose)
        printf("API v1 ready\n");
}
