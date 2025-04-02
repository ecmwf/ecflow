/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Permissions_HPP
#define ecflow_node_Permissions_HPP

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "ecflow/core/Identity.hpp"

class Variable;

namespace ecf {

enum class Allowed : std::uint8_t {
    NONE    = 0,
    READ    = 1 << 0,
    WRITE   = 1 << 1,
    EXECUTE = 1 << 2,
    OWNER   = 1 << 3,
};

inline Allowed operator|(Allowed lhs, Allowed rhs) {
    using allowed_t = std::underlying_type<Allowed>::type;

    return Allowed{static_cast<Allowed>(static_cast<allowed_t>(lhs) | static_cast<allowed_t>(rhs))};
}

inline Allowed operator&(Allowed lhs, Allowed rhs) {
    using allowed_t = std::underlying_type<Allowed>::type;

    return Allowed{static_cast<Allowed>(static_cast<allowed_t>(lhs) & static_cast<allowed_t>(rhs))};
}

inline std::string to_string(Allowed allowed) {
    std::string s;
    if ((allowed & Allowed::READ) != Allowed::NONE) {
        s += "r";
    }
    if ((allowed & Allowed::WRITE) != Allowed::NONE) {
        s += "w";
    }
    if ((allowed & Allowed::EXECUTE) != Allowed::NONE) {
        s += "x";
    }
    if ((allowed & Allowed::OWNER) != Allowed::NONE) {
        s += "o";
    }
    return s;
}

inline Allowed& operator|=(Allowed& lhs, Allowed rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline Allowed& operator&=(Allowed& lhs, Allowed rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline bool contains(Allowed lhs, Allowed rhs) {
    return (lhs & rhs) == rhs;
}

class Permission {
public:
    Permission(Username user, Allowed allowed) : username_{std::move(user)}, allowed_{allowed} {}

    [[nodiscard]] bool allows(const Username& user, Allowed requested) const {
        return user == username_ && (contains(allowed_, requested));
    }

    Username username() const { return username_; }
    Allowed allowed() const { return allowed_; }

private:
    Username username_;
    Allowed allowed_;
};

class Permissions {
public:
    static Permissions make_empty() { return Permissions(); }
    static Permissions make_from_variable(const std::string& var_value);

    static Permissions find_in(const std::vector<Variable>& variables);

    [[nodiscard]] bool is_empty() const { return allowed_.empty(); }

    [[nodiscard]] bool allows(const Username& username, Allowed permission) const {
        auto found = std::find_if(std::begin(allowed_), std::end(allowed_), [&](auto&& current) {
            return current.allows(username, permission);
        });
        return found != std::end(allowed_);
    }

private:
    Permissions() : allowed_{} {}
    explicit Permissions(std::vector<Permission> allowed) : allowed_{std::move(allowed)} {}

    std::vector<Permission> allowed_;
};

} // namespace ecf

#endif /* ecflow_node_Permissions_HPP */
