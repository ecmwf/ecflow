/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/Client.hpp"

#include <string>

#include "ecflow/core/Child.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/BasicAuth.hpp"
#include "ecflow/http/HttpServerException.hpp"
#if defined(ECF_OPENSSL)
    #include "ecflow/http/TokenStorage.hpp"
#endif

// For token based authentication, this user account
// needs to have full access to server as it's acting
// in lieu of the actual user

namespace ecf::http {

const auto ECF_USER = ecf::environment::fetch(ecf::environment::ECF_USER);
const auto ECF_PASS = ecf::environment::fetch(ecf::environment::ECF_PASS);

bool authenticate(const httplib::Request& request, ClientInvoker* ci) {

#ifdef ECF_OPENSSL
    auto auth_with_token = [&](const std::string& token) {
        if (TokenStorage::instance().verify(token)) {
            if (ECF_USER && ECF_PASS) {
                ci->set_user_name(ECF_USER.value());
                ci->set_password(ECF_PASS.value());
            }
            return true;
        }
        return false;
    };
#endif

    const auto& header = request.headers;

    // Authorization: Basic ..........
    // Authorization: Bearer ..........

    auto auth = header.find("Authorization");

    if (auth != header.end()) {
        std::vector<std::string> elems;
        ecf::Str::split(auth->second, elems, " ");

        if (elems[0] == "Basic") {
            auto [username, password] = BasicAuth::get_credentials(elems[1]);
            ci->set_user_name(username);
            ci->set_password(password);
            return true;
        }
        else if (elems[0] == "Bearer") {
#ifndef ECF_OPENSSL
            throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                      "Server compiled without SSL support");
#else
            return auth_with_token(elems[1]);
#endif
        }
        return false;
    }

    // No standard authentication header set,
    // try X-API-Key and/or queryparam

    auth = header.find("X-API-Key");

    if (auth != header.end()) {
#ifndef ECF_OPENSSL
        throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                  "Server compiled without SSL support");
#else
        return auth_with_token(auth->second);
#endif
    }

    // allow token to be passed as a query parameter
    if (const std::string token = request.get_param_value("key"); token.empty() == false) {
#ifndef ECF_OPENSSL
        throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                  "Server compiled without SSL support");
#else
        return auth_with_token(token);
#endif
    }

    return false;
}

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request) {
    auto ci = std::make_unique<ClientInvoker>();

    if (request.method != "GET" && request.method != "OPTIONS" && request.method != "HEAD" &&
        authenticate(request, ci.get()) == false) {
        throw HttpServerException(HttpStatusCode::client_error_unauthorized, "Unauthorized");
    }
    return ci;
}

std::unique_ptr<ClientInvoker> get_client_for_tasks(const httplib::Request& request, const ojson& payload) {
    auto ci = get_client(request);

    ci->set_child_path(payload.at(ecf::environment::ECF_NAME).get<std::string>());
    ci->set_child_password(payload.at(ecf::environment::ECF_PASS).get<std::string>());
    ci->set_child_pid(json_type_to_string(payload.at(ecf::environment::ECF_RID)));
    ci->set_child_try_no(std::stoi(json_type_to_string(payload.at(ecf::environment::ECF_TRYNO))));
    ci->set_child_timeout(payload.value(ecf::environment::ECF_TIMEOUT, 86400));
    ci->set_zombie_child_timeout(payload.value(ecf::environment::ECF_ZOMBIE_TIMEOUT, 43200));

    return ci;
}

} // namespace ecf::http
