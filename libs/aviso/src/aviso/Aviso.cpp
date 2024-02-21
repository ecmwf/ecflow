/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/Aviso.hpp"

#include <cassert>
#include <iostream>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace aviso {

std::string Content::decode_base64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))),
                                                [](char c) { return c == '\0'; });
}

std::string Content::decode_base64(std::string_view val) {
    std::string v{val};
    return decode_base64(v);
}

std::string Content::encode_base64(const std::string& val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

std::string Content::encode_base64(std::string_view val) {
    std::string v{val};
    return encode_base64(v);
}

Range::key_t Range::increment_last_byte(key_t val) {
    assert(!val.empty());
    val[val.size() - 1]++;
    return val;
}

Listener& Listener::with_parameter(const std::string& parameter, const std::string& value) {
    const std::string LBRACE  = R"(\{)";
    const std::string RBRACE  = R"(\})";
    const std::string pattern = LBRACE + parameter + RBRACE;

    const auto re = std::regex(pattern);

    resolved_base_ = std::regex_replace(resolved_base_, re, value);
    resolved_stem_ = std::regex_replace(resolved_stem_, re, value);

    parameters_[parameter] = value;

    return *this;
}

void Listener::listen_to(const std::string& key) const {

    // 1. turn `full` key into regex
    auto original_full = full();

    // A. Find placeholders
    // TODO can be done only once
    std::vector<std::string> placeholders;
    {
        std::regex re(R"(\{([^}]+)\})");

        auto m_bgn = std::sregex_iterator(std::begin(original_full), std::end(original_full), re);
        auto m_end = std::sregex_iterator();

        for (auto i = m_bgn; i != m_end; ++i) {
            std::smatch m = *i;
            placeholders.push_back(m[1]);
        }
    }

    // B. Create matching regex, by replacing placeholders
    // TODO can be done only once
    auto replaced_full = full();

    for (const auto& placeholder : placeholders) {
        const std::string LBRACE  = R"(\{)";
        const std::string RBRACE  = R"(\})";
        const std::string pattern = LBRACE + placeholder + RBRACE;

        const auto re = std::regex(pattern);

        const std::string value = R"(([\d\w]*))";

        replaced_full = std::regex_replace(replaced_full, re, value);
    }

    // C. Extract result from match
    auto parameters = std::vector<std::pair<std::string, std::string>>{};

    {
        const auto re = std::regex(replaced_full);

        auto m_bgn = std::sregex_iterator(std::begin(key), std::end(key), re);
        auto m_end = std::sregex_iterator();

        for (auto i = m_bgn; i != m_end; ++i) {
            std::smatch m = *i;
            // std::cout << "Listened to:" << m.str() << std::endl;
            for (size_t i = 1; i != m.size(); ++i) {
                parameters.push_back(std::make_pair(placeholders[i - 1], m[i]));
            }
        }
    }

    // D. Check parameters
    bool applicable = true;
    {
        for (const auto& [k, v] : parameters) {
            //            std::cout << "-> " << k << " = " << v << std::endl;
            if (auto found = parameters_.find(k); found != std::end(parameters_) && found->second != v) {
                applicable = false;
            }
        }
    }

    std::cout << "<Notification> " << key << std::endl;
    if (applicable) {
        std::cout << "Reference: " << original_full << std::endl;
        for (const auto& [k, v] : parameters) {
            std::cout << "-> " << k << " = " << v << std::endl;
        }
        std::cout << "Match: ✓" << std::endl << std::endl;
    }
    else {
        std::cout << "Match: ✗" << std::endl << std::endl;
    }
}

} // namespace aviso
