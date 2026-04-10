/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Base64.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr std::array<unsigned char, 64> make_encoding_table() {
    return std::array<unsigned char, 64>{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
        's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
}

constexpr auto encoding_table = make_encoding_table();

constexpr unsigned char padding_marker = '=';
constexpr uint8_t invalid_marker       = 0xFF;

constexpr std::array<uint8_t, 256> make_decoding_table() {
    std::array<uint8_t, 256> table{};
    for (auto& v : table) {
        v = invalid_marker;
    }
    for (uint8_t i = 0; i < static_cast<uint8_t>(encoding_table.size()); ++i) {
        table[encoding_table[i]] = i;
    }
    table[padding_marker] = 0;
    return table;
}

constexpr auto decoding_table = make_decoding_table();

bool is_base64_char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '+' || c == '/');
}

bool is_base64_padding(unsigned char c) {
    return c == padding_marker;
}

bool is_base64(unsigned char c) {
    return is_base64_char(c) || is_base64_padding(c);
}

} // namespace

namespace ecf {

std::string decode_base64(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    const auto len = value.size();
    if (len % 4 != 0) {
        throw std::invalid_argument("Invalid base64 input length");
    }

    // Determine output size, provisioning padding at the end when necessary
    size_t out_len = (len / 4) * 3;
    if (value[len - 1] == '=') {
        --out_len;
    }
    if (value[len - 2] == '=') {
        --out_len;

        // Detect invalid padding, with a non-equal sign after the first equals (e.g. Zg=a)
        if (value[len - 1] != '=') {
            throw std::invalid_argument("Invalid base64 padding");
        }
    }

    std::string result;
    result.reserve(out_len);

    for (size_t i = 0; i < len; i += 4) {

        auto a = static_cast<uint8_t>(value[i]);
        auto b = static_cast<uint8_t>(value[i + 1]);
        auto c = static_cast<uint8_t>(value[i + 2]);
        auto d = static_cast<uint8_t>(value[i + 3]);

        if (!is_base64(a) || !is_base64(b) || !is_base64(c) || !is_base64(d)) {
            throw std::invalid_argument("Invalid base64 input");
        }

        uint32_t sextet_a = decoding_table[a];
        uint32_t sextet_b = decoding_table[b];
        uint32_t sextet_c = decoding_table[c];
        uint32_t sextet_d = decoding_table[d];

        uint32_t triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        result += static_cast<char>((triple >> 16) & 0xFF);
        if (c != '=') {
            result += static_cast<char>((triple >> 8) & 0xFF);
        }
        if (d != '=') {
            result += static_cast<char>(triple & 0xFF);
        }
    }

    return result;
}

std::string encode_base64(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    const auto len = value.size();
    // Output size: 4 characters for every 3 input bytes, rounded up
    size_t out_len = 4 * ((len + 2) / 3);

    std::string result;
    result.reserve(out_len);

    for (size_t i = 0; i < len; i += 3) {
        auto a = static_cast<uint8_t>(value[i]);
        auto b = (i + 1 < len) ? static_cast<uint8_t>(value[i + 1]) : uint8_t{0};
        auto c = (i + 2 < len) ? static_cast<uint8_t>(value[i + 2]) : uint8_t{0};

        uint32_t triple = (static_cast<uint32_t>(a) << 16) | (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(c);

        result += encoding_table[(triple >> 18) & 0x3F];
        result += encoding_table[(triple >> 12) & 0x3F];
        result += (i + 1 < len) ? encoding_table[(triple >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? encoding_table[triple & 0x3F] : '=';
    }

    return result;
}

bool validate_base64(std::string_view value) {
    // An empty string is considered valid base64 (represents empty data)
    if (value.empty()) {
        return true;
    }

    // An invalid sized string is considered invalid base64 (must be padded with '=' to multiple of 4)
    if (value.size() % 4 != 0) {
        return false;
    }

    // Check that all characters are valid base64 characters
    // Padding ('=') is only allowed at the end (last 1 or 2 characters)
    size_t padding_start = value.size();
    for (size_t i = 0; i < value.size(); ++i) {
        auto c = static_cast<unsigned char>(value[i]);
        if (value[i] == '=') {
            padding_start = i;
            break;
        }
        if (decoding_table[c] == 0xFF) {
            return false; // invalid character
        }
    }

    // Verify padding: only 0, 1, or 2 '=' characters at the very end
    size_t padding_len = value.size() - padding_start;
    if (padding_len > 2) {
        return false;
    }
    for (size_t i = padding_start; i < value.size(); ++i) {
        if (value[i] != '=') {
            return false; // non-padding character after padding
        }
    }

    return true;
}

} // namespace ecf
