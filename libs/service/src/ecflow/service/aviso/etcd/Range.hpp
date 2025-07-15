/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_aviso_ectd_Range_HPP
#define ecflow_service_aviso_ectd_Range_HPP

#include <string>
#include <string_view>

#include "ecflow/core/Base64.hpp"

namespace ecf::service::aviso::etcd {

struct Raw;
struct Base64;

class Content {
public:
    using content_t      = std::string;
    using content_view_t = std::string_view;

    const content_t& raw() const { return raw_; }
    content_t base64() const { return ecf::encode_base64(raw_); }

    template <typename tag>
    friend Content make_content_from(Content::content_view_t content);

private:
    explicit Content(content_view_t raw) : raw_{raw} {}

    content_t raw_;
};

template <typename tag>
inline Content make_content_from(Content::content_view_t content);

template <>
inline Content make_content_from<Raw>(Content::content_view_t content) {
    return Content(content);
}

template <>
inline Content make_content_from<Base64>(Content::content_view_t content) {
    auto raw = ecf::decode_base64(std::string{content});
    return make_content_from<Raw>(raw);
}

template <typename tag>
inline Content make_content_from(const Content::content_t& content);

template <>
inline Content make_content_from<Raw>(const Content::content_t& content) {
    return make_content_from<Raw>(std::string_view{content});
}

template <>
inline Content make_content_from<Base64>(const Content::content_t& content) {
    return make_content_from<Base64>(std::string_view{content});
}

class Range {
public:
    using key_t      = std::string;
    using key_view_t = std::string_view;

    explicit Range(key_view_t key)
        : range_bgn(make_content_from<Raw>(key)),
          range_end(make_content_from<Raw>(increment_last_byte(range_bgn.raw()))) {}

    key_t base64_begin() const { return range_bgn.base64(); }
    key_t base64_end() const { return range_end.base64(); }

private:
    static key_t increment_last_byte(key_t val);

private:
    Content range_bgn;
    Content range_end;
};

} // namespace ecf::service::aviso::etcd

#endif /* ecflow_service_aviso_etcd_Range_HPP */
