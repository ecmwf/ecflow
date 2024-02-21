/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <httplib.h>
#include <iostream>

#include <nlohmann/json.hpp>

#include "aviso/Aviso.hpp"

void process_result(const aviso::Listener& listener, const httplib::Result& result) {
    using json = nlohmann::ordered_json;

    std::cout << "Received: " << result.value().status << " --> " << result.value().reason << std::endl << std::endl;

    if (result.value().status != 200) {
        std::cout << "Something when wrong!" << std::endl;
        return;
    }

    // std::cout << "Received: " << result.value().body << std::endl;

    auto response_body = json::parse(std::begin(result.value().body), std::end(result.value().body));
    // std::cout << "Received: " << response_body.dump(2) << std::endl;

    if (response_body.contains("kvs")) {
        for (const auto& kv : response_body["kvs"]) {
            auto k     = kv["key"];
            auto key   = aviso::make_from<aviso::Base64>(k);
            auto v     = kv["value"];
            auto value = aviso::make_from<aviso::Base64>(v);

            if (key.raw() == listener.prefix()) {
                auto meta_value = value.raw();
                auto meta       = json::parse(meta_value);
                std::cout << "<Header>" << std::endl;
                std::cout << "-> prev_rev = " << meta["prev_rev"] << std::endl << std::endl;
            }
            else {
                // std::cout << "Received key+value: " << key.raw() << "+" << value.raw() << std::endl;
                listener.listen_to(key.raw());
            }
        }
    }
    else {
        std::cout << "No key+value received" << std::endl;
    }
}

void process_listener(const aviso::Listener& listener, httplib::Client& client) {
    using json = nlohmann::ordered_json;

    std::string path = "/v3/kv/range";
    httplib::Headers headers;

    int min_mod_revision = 12;
    auto range           = aviso::Range(listener.prefix());

    std::string body =
        json::object({{"key", range.base64_begin()}, {"range_end", range.base64_end()}, {"min_mod_revision", min_mod_revision}}).dump();
    std::string content_type = "application/json";

    std::cout << "Sending: " << body << std::endl;

    httplib::Result result = client.Post(path, headers, body, content_type);
    process_result(listener, result);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::string scheme_host_port = "http://localhost:2379";
    httplib::Client client(scheme_host_port);

    // Aviso Attribute configuration (provided in .def file)
    // TODO: load cfg from file
    std::vector<std::string> aviso_attribute_cfgs = {
        R"({ "event": "dissemination", "request": { "destination": "COM", "target": "T9", "step": [0, 6, 10, 18] } })"
        R"({ "event": "mars", "request": { "class": "od" } })"};

    // Aviso Schema configuration (provided in .json file, or maybe downloaded from Aviso server?)
    // TODO: load schema file, and extract relevant cfg
    std::string aviso_listener_schema_cfg = {};

    auto l1 = aviso::Listener{"/ec/diss/{destination}",
                              "date={date},target={target},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}"}
                  .with_parameter("destination", "COM")
                  .with_parameter("target", "T9");

    auto l2 = aviso::Listener{"/ec/mars", "date={date},class={class},expver={expver},domain={domain},time={time},stream={stream},step={step}"}
                  .with_parameter("class", "od");

    for (auto&& listener : {l1, l2}) {
        process_listener(listener, client);
    }

    return EXIT_SUCCESS;
}
