/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/aviso/etcd/Client.hpp"

#include <iostream>

#include "ecflow/core/Message.hpp"

#if defined(ECF_OPENSSL)
    #include <openssl/ssl.h>
    #if OPENSSL_VERSION_NUMBER < 0x1010100fL
        #warning OpenSSL versions prior to 1.1.1 detected. Aviso ETCD HTTP client will be build without OpenSSL support!
    #else
        #define CPPHTTPLIB_OPENSSL_SUPPORT
    #endif
#endif

#include <cassert>
#include <fstream>
#include <httplib.h>
#include <iostream>
#include <memory>
#include <regex>

#include <nlohmann/json.hpp>

#include "ecflow/service/Log.hpp"
#include "ecflow/service/aviso/etcd/Range.hpp"

namespace ecf::service::aviso::etcd {

struct Client::Impl
{
    explicit Impl(Address address) : address_(std::move(address)), client_(std::string{address_.address()}) {}
    explicit Impl(Address address, std::string auth_token)
        : address_(std::move(address)),
          auth_token_(std::move(auth_token)),
          client_(std::string{address_.address()}) {}

    Address address_;
    std::string auth_token_;
    httplib::Client client_;
};

Client::Client(Address address) : impl_(std::make_unique<Client::Impl>(address)) {
    std::cout << OPENSSL_VERSION_NUMBER << std::endl;
}

Client::Client(Address address, std::string auth_token)
    : impl_(std::make_unique<Client::Impl>(std::move(address), std::move(auth_token))) {
}

Client::~Client() = default;

std::vector<std::pair<std::string, std::string>> Client::poll(std::string_view key_prefix, int64_t revision) {
    using json = nlohmann::ordered_json;

    httplib::Headers headers;

    if (!impl_->auth_token_.empty()) {
        ALOG(D, "EtcdClient: using authorization token");
        headers.emplace("Authorization", "Bearer " + impl_->auth_token_);
    }

    auto range = Range(key_prefix);

    std::string request_body =
        json::object({{"key", range.base64_begin()}, {"range_end", range.base64_end()}, {"min_mod_revision", revision}})
            .dump();
    std::string content_type = "application/json";

    httplib::Result result = impl_->client_.Post(endpoint_path, headers, request_body, content_type);
    if (!result) {
        throw std::runtime_error(Message("EtcdClient: Unable to retrieve result, due to ", result.error()).str());
    }

    if (result.value().status != 200) {
        throw std::runtime_error(Message("EtcdClient: Failed to poll, due to ", result.value().reason).str());
    }

    json response_body;
    try {
        response_body = json::parse(std::begin(result.value().body), std::end(result.value().body));
    }
    catch (const json::parse_error& e) {
        throw std::runtime_error(Message("EtcdClient: Unable to parse response, due to ", e.what()).str());
    }

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
                ALOG(D, "EtcdClient: Received key+value: " << key.raw() << "+" << value.raw());
                entries.emplace_back(key.raw(), value.raw());
            }
        }
    }
    else {
        ALOG(D, "EtcdClient: No new key+value found");
    }

    return entries;
}

} // namespace ecf::service::aviso::etcd
