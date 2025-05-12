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

    Permissions supersede(const Permissions& other) const {
        // `*this` is the new permissions
        // `other` is the previous permissions
        // We want to update previous permissions, by restricting them using the new permissions

        std::vector<Permission> result;

        for (auto&& new_permission : this->allowed_) {
            if (auto found =
                    std::find_if(std::begin(other.allowed_),
                                 std::end(other.allowed_),
                                 [&](auto&& current) { return current.username() == new_permission.username(); });
                found != std::end(other.allowed_)) {
                result.push_back(Permission{new_permission.username(), new_permission.allowed() & found->allowed()});
            }
        }

        return Permissions{std::move(result)};
    }

    friend std::ostream& operator<<(std::ostream& os, const Permissions& p) {
        using namespace std::string_literals;
        os << "Permissions: "s;
        for (auto&& permission : p.allowed_) {
            os << " --> "s << permission.username().value() << ":"s << to_string(permission.allowed()) << " "s;
        }
        return os;
    }

private:
    Permissions() : allowed_{} {}
    explicit Permissions(std::vector<Permission> allowed) : allowed_{std::move(allowed)} {}

    std::vector<Permission> allowed_;
};

class ActivePermissions {
public:
    static ActivePermissions make_empty() { return ActivePermissions(); }

    [[nodiscard]] bool is_none() const { return server_permissions_.is_empty() && node_permissions_.is_empty(); }

    [[nodiscard]] bool allows(const Username& username, Allowed permission) const {
        if (is_none()) {
            return true; // no active rules, so allow everything
        }

        if (!server_permissions_.is_empty() && server_permissions_.allows(username, permission)) {
            return true;
        }

        if (!node_permissions_.is_empty() && node_permissions_.allows(username, permission)) {
            return true;
        }

        return false;
    }

    void bootstrap_server_permission(const Permissions& p) { server_permissions_ = p; }

    void bootstrap_node_permission(const Permissions& p) { node_permissions_ = p; }

    void combine_node_permission(const Permissions& p) {
        if (!p.is_empty()) {
            node_permissions_ = p.supersede(node_permissions_);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const ActivePermissions& p) {
        using namespace std::string_literals;
        os << "ActivePermissions\n"s;
        os << " -- Server\n"s;
        os << p.server_permissions_;
        os << "\n -- Node\n"s;
        os << p.node_permissions_;
        return os;
    }

private:
    Permissions server_permissions_ = Permissions::make_empty();
    Permissions node_permissions_   = Permissions::make_empty();
};

} // namespace ecf

#endif /* ecflow_node_Permissions_HPP */
