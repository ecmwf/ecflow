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
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "ecflow/core/Identity.hpp"

class Variable;

namespace ecf {

enum class Allowed : std::uint8_t {
    NONE    = 0,
    READ    = 1 << 0, // Can perform read commands on nodes and their attributes
    WRITE   = 1 << 1, // Can perform write commands on nodes and their attributes
    EXECUTE = 1 << 2, // Can perform execute commands on nodes (e.g. run a task)
    OWNER   = 1 << 3, // Can to load new suites.
    STICKY  = 1 << 4, // This is a special permission and means the permission is cannot be restricted/removed in lower
                      // hierarchy levels, i.e. if a user has sticky permissions, it can't be overridden
                      // by a new permission defined at a lower level. This is useful for defining
                      // server level permissions that should not be overridden by node level permissions.
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
    if ((allowed & Allowed::STICKY) != Allowed::NONE) {
        s += "s";
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

    static Permissions combine_supersede(const Permissions& active, const Permissions& current) {
        // `current` is the new permissions
        // `previous` is the previous permissions
        //
        // The superseding rules are to update previous permissions, by keeping all sticky permissions and keeping any
        // non-sticky permissions with the new permissions.
        //

        std::vector<Permission> result;

        std::cout << "Combining permissions (supersede) active: " << active << " current: " << current << std::endl;

        // First, we keep all sticky permissions from the active permissions
        for (auto&& permission : active.allowed_) {
            if (contains(permission.allowed(), Allowed::STICKY)) {
                result.push_back(permission);
            }
        }

        // Then, we retain the non-sticky permissions from the current permissions
        for (auto&& permission : current.allowed_) {
            auto found = std::find_if(
                std::begin(result), std::end(result), [&](auto&& p) { return p.username() == permission.username(); });

            if (found == std::end(result)) {
                // If the user is not in listed as sticky, we add the permission
                result.push_back(permission);
            }
        }

        auto r = Permissions{std::move(result)};
        std::cout << "Combined permissions (supersede): " << r << std::endl;
        return r;
    }

    static Permissions combine_override(const Permissions& active, const Permissions& current) {
        // `current` is the new permissions
        // `previous` is the previous permissions
        //
        // The overriding rules are to update previous permissions, by restricting them based on the new permissions.
        // Sticky permissions are kept as is, while non-sticky permissions are overridden by the current permissions.
        //

        std::vector<Permission> result;

        for (auto&& permission : active.allowed_) {
            if (contains(permission.allowed(), Allowed::STICKY)) {
                // If the permission is sticky, we keep the permissions as is
                result.push_back(permission);
            }
            else {
                if (auto found = std::find_if(std::begin(current.allowed_),
                                              std::end(current.allowed_),
                                              [&](auto&& p) { return p.username() == permission.username(); });
                    found != std::end(current.allowed_)) {
                    // When non-sticky permissions are overridden in current, we combine the permissions
                    result.push_back(Permission{permission.username(), permission.allowed() & found->allowed()});
                }
            }
        }

        return Permissions{std::move(result)};
    }

    friend std::ostream& operator<<(std::ostream& os, const Permissions& p) {
        using namespace std::string_literals;
        os << "Permissions: "s;
        for (auto&& permission : p.allowed_) {
            os << "("s << permission.username().value() << ":"s << to_string(permission.allowed()) << ") "s;
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

    [[nodiscard]] bool is_none() const { return permissions_.is_empty(); }

    [[nodiscard]] bool allows(const Username& username, Allowed permission) const {
        if (is_none()) {
            return true; // no active rules, so allow everything! Dangerous, but backward compatible!
        }

        return permissions_.allows(username, permission);
    }

    void bootstrap(const Permissions& p) { permissions_ = p; }

    void combine_supersede(const Permissions& p) {
        if (!is_none()) {
            permissions_ = Permissions::combine_supersede(permissions_, p);
        }
    }

    void combine_override(const Permissions& p) {
        if (!is_none()) {
            permissions_ = Permissions::combine_override(permissions_, p);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const ActivePermissions& p) {
        using namespace std::string_literals;
        os << "ActivePermissions\n"s;
        os << p.permissions_;
        return os;
    }

private:
    Permissions permissions_ = Permissions::make_empty();
};

} // namespace ecf

#endif /* ecflow_node_Permissions_HPP */
