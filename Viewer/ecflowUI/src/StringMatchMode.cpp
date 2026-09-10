/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "StringMatchMode.hpp"

std::map<StringMatchMode::Mode, std::string> StringMatchMode::matchOper_;

StringMatchMode::StringMatchMode() {
    init();
}

StringMatchMode::StringMatchMode(Mode mode)
    : mode_(mode) {
    init();
}

StringMatchMode::StringMatchMode(int idx)
    : mode_(static_cast<Mode>(idx)) {
    init();
}

void StringMatchMode::init() {
    if (matchOper_.empty()) {
        matchOper_[ContainsMatch] = "~";
        matchOper_[WildcardMatch] = "=";
        matchOper_[RegexpMatch]   = "=~";
    }
}

const std::string& StringMatchMode::matchOperator() const {
    static std::string emptyStr;
    auto it = matchOper_.find(mode_);
    if (it != matchOper_.end()) {
        return it->second;
    }

    return emptyStr;
}

StringMatchMode::Mode StringMatchMode::operToMode(const std::string& op) {
    for (auto it = matchOper_.begin(); it != matchOper_.end(); ++it) {
        if (op == it->second) {
            return it->first;
        }
    }
    return InvalidMatch;
}
