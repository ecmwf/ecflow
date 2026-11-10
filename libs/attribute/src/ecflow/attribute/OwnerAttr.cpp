// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/attribute/OwnerAttr.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/cereal_optional_nvp.hpp"

namespace ecf {

namespace {

constexpr std::size_t max_name_length = 32;

bool is_valid_name(const std::string& name) {
    if (name.empty() || name.size() > max_name_length) {
        return false;
    }
    if (name.front() == '-') {
        return false;
    }
    // Explicit ASCII ranges, so that the validation does not depend on the locale of the process
    auto is_digit     = [](char c) { return c >= '0' && c <= '9'; };
    auto is_name_char = [&is_digit](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_digit(c) || c == '.' || c == '_' || c == '-';
    };
    if (!std::all_of(name.begin(), name.end(), is_name_char)) {
        return false;
    }
    return !std::all_of(name.begin(), name.end(), is_digit);
}

/// Splits `user[:group]` into its parts; returns false if the value is not valid.
bool split(const std::string& value, std::string& user, std::string& group) {
    auto colon = value.find(':');
    if (colon == std::string::npos) {
        user  = value;
        group = std::string{};
        return is_valid_name(user);
    }
    user  = value.substr(0, colon);
    group = value.substr(colon + 1);
    return is_valid_name(user) && is_valid_name(group);
}

} // namespace

OwnerAttr::OwnerAttr(std::string user, std::string group)
    : user_(std::move(user)),
      group_(std::move(group)) {
}

OwnerAttr OwnerAttr::create(const std::string& value) {
    std::string user;
    std::string group;
    if (!split(value, user, group)) {
        throw std::runtime_error(
            "OwnerAttr::create: Invalid owner '" + value +
            "': expected user[:group], with names of 1 to 32 characters among letters, digits, '.', '_' and '-', "
            "not starting with '-', and not numeric");
    }
    return OwnerAttr{std::move(user), std::move(group)};
}

bool OwnerAttr::is_valid(const std::string& value) noexcept {
    try {
        std::string user;
        std::string group;
        return split(value, user, group);
    }
    catch (...) {
        return false;
    }
}

std::optional<std::string> OwnerAttr::group() const {
    if (group_.empty()) {
        return std::nullopt;
    }
    return group_;
}

std::string OwnerAttr::value() const {
    if (group_.empty()) {
        return user_;
    }
    return user_ + ":" + group_;
}

std::string OwnerAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void OwnerAttr::write(std::string& ret) const {
    ret += "owner ";
    ret += value();
}

bool OwnerAttr::operator==(const OwnerAttr& rhs) const {
    return user_ == rhs.user_ && group_ == rhs.group_;
}

template <class Archive>
void OwnerAttr::serialize(Archive& ar) {
    if constexpr (Archive::is_loading::value) {
        group_.clear(); // the group is optional in the archive, and must not survive from a previous value
    }
    ar(CEREAL_NVP(user_));
    CEREAL_OPTIONAL_NVP(ar, group_, [this]() { return !group_.empty(); });
    if constexpr (Archive::is_loading::value) {
        if (!is_valid(value())) {
            throw std::runtime_error("OwnerAttr::serialize: Invalid owner '" + value() + "' in archive");
        }
    }
}
CEREAL_TEMPLATE_SPECIALIZE(OwnerAttr);

} // namespace ecf
