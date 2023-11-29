/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "StringSplitter.hpp"

namespace ecf {

bool StringSplitter::finished() const {
    // Skip delimiters at beginning. so for an empty string or string of just separators, we do not call next()
    // This mirrors the old split functionality
    if (finished_)
        return true;
    first_not_of_ = rem_.find_first_not_of(sep_);
    if (first_not_of_ == std::string_view::npos)
        finished_ = true;
    return finished_;
}

std::string_view StringSplitter::next() const {
    if (first_not_of_ != 0)
        rem_ = rem_.substr(first_not_of_);

    std::string_view::size_type pos = rem_.find_first_of(sep_); // Find first "non-delimiter".
    if (pos != std::string_view::npos) {
        std::string_view ret = rem_.substr(0, pos);
        rem_                 = rem_.substr(pos + 1); // skip over separator

        // if separator is at the end, lose the last empty tokens
        if (rem_.find_first_not_of(sep_) == std::string_view::npos) {
            finished_ = true;
        }
        return ret;
    }

    finished_ = true;
    return rem_;
}

void StringSplitter::reset() {
    rem_      = src_;
    finished_ = false;
}

void StringSplitter::split(const std::string& str,
                           std::vector<std::string_view>& lineTokens,
                           std::string_view delimiters) {
    StringSplitter string_splitter(str, delimiters);
    while (!string_splitter.finished()) {
        lineTokens.push_back(string_splitter.next());
    }
}

void StringSplitter::split2(std::string_view str, std::vector<std::string_view>& ret, const char* delims) {
    std::string_view::size_type start = 0;
    auto pos                          = str.find_first_of(delims, start);
    while (pos != std::string_view::npos) {
        if (pos != start) {
            ret.emplace_back(str.substr(start, pos - start));
        }
        start = pos + 1;
        pos   = str.find_first_of(delims, start);
    }
    if (start < str.length())
        ret.emplace_back(str.substr(start, str.length() - start));
}

void StringSplitter::split(const std::string& str, std::vector<std::string>& lineTokens, std::string_view delimiters) {
    StringSplitter string_splitter(str, delimiters);
    while (!string_splitter.finished()) {
        std::string_view ref = string_splitter.next();
        lineTokens.emplace_back(ref.begin(), ref.end());
    }
}

bool StringSplitter::get_token(std::string_view line, size_t pos, std::string& token, std::string_view sep) {
    size_t count = 0;
    StringSplitter string_splitter(line, sep);
    while (!string_splitter.finished()) {
        std::string_view the_token = string_splitter.next();
        if (count == pos) {
            token = std::string(the_token.begin(), the_token.end());
            return true;
        }
        else if (count > pos)
            return false;
        count++;
    }
    return false;
}

} // namespace ecf
