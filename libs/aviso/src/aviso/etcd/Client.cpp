/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/etcd/Client.hpp"

#if defined(ECF_OPENSSL)
    #define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <cassert>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <regex>

#include <nlohmann/json.hpp>

#include "aviso/etcd/Range.hpp"

namespace aviso::etcd {

struct Client::Impl
{
    explicit Impl(Address address) : address_(std::move(address)), client_(std::string{address_.address()}) {}

    Address address_;
    httplib::Client client_;
};

Client::Client(const Address& address) : impl_(std::make_unique<Client::Impl>(address)) {
}

Client::~Client() = default;

std::vector<std::pair<std::string, std::string>> Client::poll(std::string_view key_prefix, int64_t revision) {
    using json = nlohmann::ordered_json;

    std::cout << "Polling key prefix: " << key_prefix;
    std::cout << ", from revision: " << revision;
    std::cout << " @ address : " << impl_->address_.address() << std::endl;

    httplib::Headers headers;

    auto range = Range(key_prefix);

    std::string request_body =
        json::object({{"key", range.base64_begin()}, {"range_end", range.base64_end()}, {"min_mod_revision", revision}})
            .dump();
    std::string content_type = "application/json";

    // std::cout << "Sending: " << request_body << std::endl;

    httplib::Result result = impl_->client_.Post(endpoint_path, headers, request_body, content_type);
    if (!result) {
        std::cout << "Error: " << result.error() << std::endl << std::endl;
        return std::vector<std::pair<std::string, std::string>>{};
    }
    std::cout << "Received: " << result.value().status << " --> " << result.value().reason << std::endl;

    if (result.value().status != 200) {
        // TODO: handle the error (either by throwing an exception or returning an optional)
        std::cout << "Something when wrong!" << std::endl;
        return std::vector<std::pair<std::string, std::string>>{};
    }

    auto response_body = json::parse(std::begin(result.value().body), std::end(result.value().body));
    // std::cout << "Received: " << response_body.dump(2) << std::endl;

    std::vector<std::pair<std::string, std::string>> entries;

    if (response_body.contains("header")) {
        auto latest_revision = response_body["header"]["revision"];
        entries.emplace_back("latest_revision", latest_revision);
    }

    if (response_body.contains("kvs")) {
        for (const auto& kv : response_body["kvs"]) {
            auto k     = kv["key"];
            auto key   = make_content_from<Base64>(k);
            auto v     = kv["value"];
            auto value = make_content_from<Base64>(v);

            if (key.raw() != key_prefix) {
                // std::cout << "Received key+value: " << key.raw() << "+" << value.raw() << std::endl;
                entries.emplace_back(key.raw(), value.raw());
            }
        }
    }
    else {
        std::cout << "No new key+value" << std::endl << std::endl;
    }

    return entries;
}

} // namespace aviso::etcd
