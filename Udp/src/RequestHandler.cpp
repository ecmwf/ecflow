/*
 * Copyright 2009-2023 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "RequestHandler.hpp"

#include <string>

#include <nlohmann/json.hpp>

#include "ClientAPI.hpp"
#include "Trace.hpp"

namespace ecf {

namespace /* __anonymous__ */ {

class JSONRequestHandler {
public:
    JSONRequestHandler() : client_{} {}

    void handle(const nlohmann::json& request) {
        try {
            std::string method_type = request.at("method");

            // process "header"
            if (request.contains("header")) {
                nlohmann::json header = request.at("header");
                std::string username  = header.at("username");
                std::string password  = header.at("password");
                client_.set_authentication(username, password);
            }

            // process "data"
            if (method_type == "put") {
                nlohmann::json data = request.at("data");
                handle_update(data);
            }
            else {
                TRACE_ERR("JSONRequestHandler", "unknown method: ", method_type)
            }
        }
        catch (nlohmann::json::exception& e) {
            TRACE_ERR("JSONRequestHandler", "JSON format error: ", e.what())
            return;
        }
        catch (ClientAPIException& e) {
            TRACE_ERR("JSONRequestHandler", "Client invocation error: ", e.what())
            return;
        }
        catch (...) {
            TRACE_ERR("JSONRequestHandler", "Unknown error detected handling request")
            return;
        }

        TRACE_NFO("JSONRequestHandler", "request handled successfully");
    }

    void handle_update(const nlohmann::json& data) {
        std::string type = data.at("type");

        if (type == "label") {
            handle_update_label(data);
        }
        else if (type == "meter") {
            handle_update_meter(data);
        }
        else if (type == "event") {
            handle_update_event(data);
        }
        else {
            TRACE_NFO("JSONRequestHandler", "unknown update type detected: '", type, "'")
        }
    }

private:
    void handle_update_label(const nlohmann::json& data) {
        std::string path  = data.at("path");
        std::string name  = data.at("name");
        std::string value = data.at("value");

        TRACE_NFO("JSONRequestHandler", "request to update label '", name, "' at '", path, "' to value '", value, "'")

        client_.update_label(path, name, value);
    }

    void handle_update_meter(const nlohmann::json& data) {
        std::string path  = data.at("path");
        std::string name  = data.at("name");
        std::string value = data.at("value");

        TRACE_NFO("JSONRequestHandler", "request to update meter '", name, "' at '", path, "' to value '", value, "'")

        client_.update_meter(path, name, value);
    }

    void handle_update_event(const nlohmann::json& data) {
        std::string path  = data.at("path");
        std::string name  = data.at("name");
        std::string value = data.at("value");

        TRACE_NFO("JSONRequestHandler", "request to update event '", name, "' at '", path, "' to value '", value, "'")

        if (value == "set") {
            client_.set_event(path, name);
        }
        else if (value == "clear") {
            client_.clear_event(path, name);
        }
        else {
            TRACE_ERR("RequestHandler", "unexpected event value found: '", value, "'");
        }
    }

private:
    ClientAPI client_;
};

} // namespace

void RequestHandler::handle(const RequestHandler::inbound_t& request) const {
    try {
        nlohmann::json inbound = nlohmann::json::parse(request);
        JSONRequestHandler handler;
        handler.handle(inbound);
    }
    catch (nlohmann::json::exception& e) {
        TRACE_ERR("RequestHandler", "Unable to parse JSON request");
    }
    catch (...) {
        TRACE_ERR("RequestHandler", "Unknown error detected");
    }
}

} // namespace ecf
