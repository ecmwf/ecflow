/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Str.hpp"

#include "ecflow/core/Converter.hpp"

namespace ecf {

namespace algorithm {

inline bool is_valid_node_name_first_character(char c) {
    return std::isalnum(c) || c == '_';
}

inline bool is_valid_node_name_following_character(char c) {
    return std::isalnum(c) || c == '_' || c == '.';
}

bool is_valid_name(const std::string& name, std::string& error) {

    if (name.empty()) {
        error = "Invalid name '': empty string";
        return false;
    }

    if (!is_valid_node_name_first_character(name.front())) {
        error = "Invalid name '";
        error += name;
        error += "': only alphanumeric characters or underscore are accepted as first character";
        return false;
    }

    if (!std::all_of(name.begin() + 1, name.end(), is_valid_node_name_following_character)) {
        error = "Invalid name '";
        error += name;
        error += "': only alphanumeric characters, underscore or dot are accepted";
        return false;
    }

    return true;
}

bool is_valid_name(const std::string& name) {
    return !name.empty() && is_valid_node_name_first_character(name.front()) &&
           std::all_of(name.begin() + 1, name.end(), is_valid_node_name_following_character);
}

bool tail(std::string& fileContents, size_t max_lines) {
    if (fileContents.empty()) {
        return false;
    }

    size_t count = 0;
    auto it      = fileContents.end();
    while (it != fileContents.begin()) {
        --it;
        if (*it == '\n') {
            ++count;
            if (count == max_lines) {
                fileContents.erase(fileContents.begin(), it + 1);
                return true;
            }
        }
    }
    return false;
}

bool head(std::string& fileContents, size_t max_lines) {
    if (fileContents.empty()) {
        return false;
    }

    size_t count = 0;
    for (size_t i = 0; i < fileContents.size(); ++i) {
        if (fileContents[i] == '\n') {
            ++count;
            if (count == max_lines) {
                fileContents.erase(i + 1);
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string_view> split_within_quotes(const std::string& input, std::string_view quotes) {

    std::vector<std::string_view> tokens;

    std::string levels;

    const char* current = &input[0];
    const char* start   = current;
    while (*current != 0) {
        if (*current == ' ' && levels.empty()) {
            if (start != current) {
                tokens.emplace_back(start, static_cast<size_t>(current - start));
            }
            start = current + 1;
        }
        else {
            if (std::any_of(
                    std::begin(quotes), std::end(quotes), [&current](char quote) { return *current == quote; })) {
                if (!levels.empty() && (levels.back() == *current)) {
                    levels.pop_back();
                }
                else {
                    if (levels.empty()) {
                        start = current;
                    }
                    levels.push_back(*current);
                }
            }
        }
        ++current;
    }

    if (start != current) {
        tokens.emplace_back(start, static_cast<size_t>(current - start));
    }

    return tokens;
}

bool get_token(std::string_view input, size_t index, std::string& token, std::string_view delimiters) {

    size_t current_index = 0;
    auto first           = std::cbegin(input);
    auto end             = std::cend(input);

    while (first != end) {
        const auto second = std::find_first_of(first, end, std::cbegin(delimiters), std::cend(delimiters));

        if (first != second) {
            if (current_index == index) {
                token = std::string(first, second);
                return true;
            }
            current_index++;
        }

        if (second == end) {
            break;
        }

        first = std::next(second);
    }
    return false;
}

} // namespace algorithm

} // namespace ecf
