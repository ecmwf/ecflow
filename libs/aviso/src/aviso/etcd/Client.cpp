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

#include "aviso/Log.hpp"
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

    httplib::Headers headers;

    auto range = Range(key_prefix);

    std::string request_body =
        json::object({{"key", range.base64_begin()}, {"range_end", range.base64_end()}, {"min_mod_revision", revision}})
            .dump();
    std::string content_type = "application/json";

    httplib::Result result = impl_->client_.Post(endpoint_path, headers, request_body, content_type);
    if (!result) {
        ALOG(E, "Error: " << result.error());
        return std::vector<std::pair<std::string, std::string>>{};
    }
    // ALOG(D, "Received: " << result.value().status << " --> " << result.value().reason);

    if (result.value().status != 200) {
        // TODO[MB]: handle the error (either by throwing an exception or returning an optional)
        ALOG(E, "Something when wrong!");
        return std::vector<std::pair<std::string, std::string>>{};
    }

    auto response_body = json::parse(std::begin(result.value().body), std::end(result.value().body));

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
                ALOG(D, "Received key+value: " << key.raw() << "+" << value.raw());
                entries.emplace_back(key.raw(), value.raw());
            }
        }
    }
    else {
        ALOG(D, "No new key+value");
    }

    return entries;
}

} // namespace aviso::etcd
