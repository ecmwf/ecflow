/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_Aviso_HPP
#define aviso_Aviso_HPP

#include <string>
#include <unordered_map>

namespace aviso {

struct Raw;
struct Base64;

class Content {
public:
    using content_t      = std::string;
    using content_view_t = std::string_view;

    const content_t& raw() const { return raw_; }
    content_t base64() const { return encode_base64(raw_); }

    template <typename tag>
    friend Content make_from(Content::content_view_t content);

private:
    static std::string decode_base64(const std::string& val);
    static std::string decode_base64(std::string_view val);

    static std::string encode_base64(const std::string& val);
    static std::string encode_base64(std::string_view val);

private:
    explicit Content(content_view_t raw) : raw_{raw} {}

    content_t raw_;
};

template <typename tag>
inline Content make_from(Content::content_view_t content);

template <>
inline Content make_from<Raw>(Content::content_view_t content) {
    return Content(content);
}

template <>
inline Content make_from<Base64>(Content::content_view_t content) {
    auto raw = Content::decode_base64(content);
    return make_from<Raw>(raw);
}

template <typename tag>
inline Content make_from(const Content::content_t& content);

template <>
inline Content make_from<Raw>(const Content::content_t& content) {
    return make_from<Raw>(std::string_view{content});
}

template <>
inline Content make_from<Base64>(const Content::content_t& content) {
    return make_from<Base64>(std::string_view{content});
}

class Range {
public:
    using key_t      = std::string;
    using key_view_t = std::string_view;

    explicit Range(key_view_t key)
        : range_bgn(make_from<Raw>(key)),
          range_end(make_from<Raw>(increment_last_byte(range_bgn.raw()))) {}

    key_t base64_begin() const { return range_bgn.base64(); }
    key_t base64_end() const { return range_end.base64(); }

private:
    static key_t increment_last_byte(key_t val);

private:
    Content range_bgn;
    Content range_end;
};

class Listener {
public:
    Listener(std::string_view base, std::string_view stem)
        : base_{base},
          resolved_base_(base),
          stem_{stem},
          resolved_stem_(stem) {}

    std::string_view base() const { return base_; }
    std::string_view stem() const { return stem_; }

    std::string_view resolved_base() const { return base_; }
    std::string_view resolved_stem() const { return stem_; }

    std::string prefix() const { return resolved_base_ + '/'; }

    std::string full() const { return base_ + '/' + stem_; }

    Listener& with_parameter(const std::string& parameter, const std::string& value);

    void listen_to(const std::string& key) const;

private:
    std::string base_;
    std::string resolved_base_;
    std::string stem_;
    std::string resolved_stem_;

    std::unordered_map<std::string, std::string> parameters_ = {};
};

} // namespace aviso

#endif /* aviso_Aviso_HPP */
