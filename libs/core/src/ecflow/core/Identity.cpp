// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/Identity.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>

namespace ecf {

namespace {

// Decodes the UTF-8 sequence starting at text[i]; returns the code point and advances i, or returns nothing (leaving
// i unchanged) when the sequence is not valid UTF-8: no overlong form, no surrogate, nothing above U+10FFFF
std::optional<char32_t> decode(std::string_view text, std::size_t& i) {
    const auto c = static_cast<unsigned char>(text[i]);
    if (c < 0x80) {
        ++i;
        return c;
    }
    std::size_t length = 0;
    char32_t code      = 0;
    char32_t min       = 0;
    if ((c & 0xE0) == 0xC0) {
        length = 2, code = c & 0x1F, min = 0x80;
    }
    else if ((c & 0xF0) == 0xE0) {
        length = 3, code = c & 0x0F, min = 0x800;
    }
    else if ((c & 0xF8) == 0xF0) {
        length = 4, code = c & 0x07, min = 0x10000;
    }
    else {
        return std::nullopt;
    }
    if (i + length > text.size()) {
        return std::nullopt;
    }
    for (std::size_t k = 1; k < length; ++k) {
        const auto cc = static_cast<unsigned char>(text[i + k]);
        if ((cc & 0xC0) != 0x80) {
            return std::nullopt;
        }
        code = (code << 6) | (cc & 0x3F);
    }
    if (code < min || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        return std::nullopt;
    }
    i += length;
    return code;
}

// The characters with the Unicode property White_Space
bool is_white_space(char32_t c) {
    return (c >= 0x0009 && c <= 0x000D) || c == 0x0020 || c == 0x0085 || c == 0x00A0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000;
}

// The characters of the general category Cc
bool is_control(char32_t c) {
    return c <= 0x001F || (c >= 0x007F && c <= 0x009F);
}

std::string code_point(char32_t c) {
    std::array<char, 16> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "<U+%04X>", static_cast<unsigned>(c));
    return buffer.data();
}

} // namespace

std::string printable_user_name(std::string_view name) {
    std::string out = "'";
    std::size_t i   = 0;
    while (i < name.size()) {
        const std::size_t start = i;
        if (auto c = decode(name, i); c) {
            if (is_white_space(*c) || is_control(*c)) {
                out += code_point(*c);
            }
            else {
                out.append(name.substr(start, i - start));
            }
        }
        else {
            std::array<char, 8> buffer{};
            std::snprintf(buffer.data(), buffer.size(), "<0x%02X>", static_cast<unsigned char>(name[i]));
            out += buffer.data();
            ++i;
        }
    }
    return out + "'";
}

std::string printable_text(std::string_view text) {
    std::string out;
    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t start = i;
        if (auto c = decode(text, i); c) {
            if (is_control(*c)) {
                out += code_point(*c);
            }
            else {
                out.append(text.substr(start, i - start));
            }
        }
        else {
            std::array<char, 8> buffer{};
            std::snprintf(buffer.data(), buffer.size(), "<0x%02X>", static_cast<unsigned char>(text[i]));
            out += buffer.data();
            ++i;
        }
    }
    return out;
}

bool is_valid_user_name(std::string_view name, std::string& error) {
    if (name.empty()) {
        error = "user name '' is empty";
        return false;
    }
    if (name == "*") {
        error = "user name '*' is not allowed";
        return false;
    }
    if (name.front() == '-') {
        error = "user name " + printable_user_name(name) + " starts with '-'";
        return false;
    }
    std::size_t i = 0;
    while (i < name.size()) {
        const auto c = decode(name, i);
        if (!c) {
            error = "user name " + printable_user_name(name) + " is not valid UTF-8";
            return false;
        }
        if (is_white_space(*c)) {
            error = "user name " + printable_user_name(name) + " contains white space (" + code_point(*c) + ")";
            return false;
        }
        if (is_control(*c)) {
            error = "user name " + printable_user_name(name) + " contains a control character (" + code_point(*c) + ")";
            return false;
        }
        if (*c == '#' || *c == ',' || *c == ':' || *c == ';') {
            error = "user name " + printable_user_name(name) + " contains '" + static_cast<char>(*c) + "'";
            return false;
        }
    }
    return true;
}

} // namespace ecf
