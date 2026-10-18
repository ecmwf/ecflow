// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/MaskCredentials.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace ecf {

namespace {

constexpr std::string_view mask = "***";

// A range [first, last) of the value to replace by the mask
using Span = std::pair<std::size_t, std::size_t>;

bool is_name_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.';
}

bool is_space(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

// True when one of the parts of the name, split at '_', '-' or '.', is a credential word (in any case)
bool is_credential_name(std::string_view name) {
    static const std::array<std::string_view, 5> words = {"password", "passwd", "token", "secret", "key"};

    std::size_t start = 0;
    while (start <= name.size()) {
        auto end = name.find_first_of("_-.", start);
        if (end == std::string_view::npos) {
            end = name.size();
        }
        auto part = name.substr(start, end - start);
        for (auto word : words) {
            if (part.size() == word.size() && std::equal(part.begin(), part.end(), word.begin(), [](char a, char b) {
                    return std::tolower(static_cast<unsigned char>(a)) == b;
                })) {
                return true;
            }
        }
        start = end + 1;
    }
    return false;
}

// The password of each URL: scheme://user:password@host
void find_url_passwords(std::string_view value, std::vector<Span>& spans) {
    for (auto scheme_end = value.find("://"); scheme_end != std::string_view::npos;
         scheme_end      = value.find("://", scheme_end + 3)) {
        auto authority = scheme_end + 3;
        auto end       = authority;
        while (end < value.size() && !is_space(value[end]) && value[end] != '/' && value[end] != '?' &&
               value[end] != '#') {
            ++end;
        }
        auto at = value.rfind('@', end == 0 ? 0 : end - 1);
        if (at == std::string_view::npos || at < authority) {
            continue;
        }
        auto colon = value.find(':', authority);
        if (colon != std::string_view::npos && colon < at) {
            spans.emplace_back(colon + 1, at);
        }
    }
}

// A shell word: [first, last), quotes and escapes included; an unterminated quote extends it to the end
struct Word
{
    std::size_t first;
    std::size_t last;
};

std::vector<Word> split_words(std::string_view value) {
    std::vector<Word> words;
    std::size_t i = 0;
    while (i < value.size()) {
        while (i < value.size() && is_space(value[i])) {
            ++i;
        }
        if (i == value.size()) {
            break;
        }
        auto first = i;
        char quote = 0;
        while (i < value.size() && (quote || !is_space(value[i]))) {
            char c = value[i];
            if (quote == '\'') {
                quote = (c == '\'') ? 0 : quote;
            }
            else if (c == '\\') {
                ++i; // the next character is escaped
            }
            else if (quote == '"') {
                quote = (c == '"') ? 0 : quote;
            }
            else if (c == '\'' || c == '"') {
                quote = c;
            }
            ++i;
        }
        i = std::min(i, value.size());
        words.push_back({first, i});
    }
    return words;
}

// The value of each name=value pair of the word, ending at the first unquoted '&' (as in a URL query)
void find_named_keys(std::string_view value, const Word& word, std::vector<Span>& spans) {
    std::size_t i = word.first;
    char quote    = 0;
    while (i < word.last) {
        char c = value[i];
        if (quote == '\'') {
            quote = (c == '\'') ? 0 : quote;
        }
        else if (c == '\\') {
            ++i;
        }
        else if (quote == '"') {
            quote = (c == '"') ? 0 : quote;
        }
        else if (c == '\'' || c == '"') {
            quote = c;
        }
        else if (c == '=') {
            auto name_first = i;
            while (name_first > word.first && is_name_char(value[name_first - 1])) {
                --name_first;
            }
            auto name = value.substr(name_first, i - name_first);
            while (!name.empty() && name.front() == '-') {
                name.remove_prefix(1);
            }
            // The value ends at the first unquoted '&', or at the end of the word
            auto value_first = i + 1;
            auto j           = value_first;
            char value_quote = 0;
            while (j < word.last && (value_quote || value[j] != '&')) {
                char v = value[j];
                if (value_quote == '\'') {
                    value_quote = (v == '\'') ? 0 : value_quote;
                }
                else if (v == '\\') {
                    ++j;
                }
                else if (value_quote == '"') {
                    value_quote = (v == '"') ? 0 : value_quote;
                }
                else if (v == '\'' || v == '"') {
                    value_quote = v;
                }
                ++j;
            }
            j = std::min(j, word.last);
            if (!name.empty() && is_credential_name(name) && value_first < j) {
                spans.emplace_back(value_first, j);
            }
            i = j;
            continue;
        }
        ++i;
    }
}

// True when the word is an option -name or --name, whose name is a credential name
bool is_credential_option(std::string_view word) {
    if (word.size() < 2 || word.front() != '-') {
        return false;
    }
    auto name = word.substr(word[1] == '-' ? 2 : 1);
    return !name.empty() && std::all_of(name.begin(), name.end(), is_name_char) && is_credential_name(name);
}

} // namespace

std::string mask_credentials(std::string_view value) {
    std::vector<Span> spans;

    find_url_passwords(value, spans);

    auto words = split_words(value);
    for (std::size_t w = 0; w < words.size(); ++w) {
        find_named_keys(value, words[w], spans);
        if (w + 1 < words.size() &&
            is_credential_option(value.substr(words[w].first, words[w].last - words[w].first))) {
            spans.emplace_back(words[w + 1].first, words[w + 1].last);
        }
    }

    // Replace the spans, merged where they overlap, in a single pass
    std::sort(spans.begin(), spans.end());
    std::string masked;
    masked.reserve(value.size());
    std::size_t copied = 0;
    for (std::size_t s = 0; s < spans.size();) {
        auto first = spans[s].first;
        auto last  = spans[s].second;
        for (++s; s < spans.size() && spans[s].first <= last; ++s) {
            last = std::max(last, spans[s].second);
        }
        if (first < copied) {
            continue;
        }
        masked.append(value.substr(copied, first - copied));
        masked.append(mask);
        copied = last;
    }
    masked.append(value.substr(copied));
    return masked;
}

} // namespace ecf
