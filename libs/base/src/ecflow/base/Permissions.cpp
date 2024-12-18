/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/Permissions.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "ecflow/core/WhiteListFile.hpp"

namespace ecf {

template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

struct Rule
{
    Rule(std::string pattern,
         std::vector<std::string> allowed_users,
         std::vector<std::string> allowed_roles,
         std::vector<std::string> permissions)
        : pattern_{std::move(pattern)},
          regex_{pattern_},
          allowed_users_{std::move(allowed_users)},
          allowed_roles_{std::move(allowed_roles)},
          permissions_{std::move(permissions)} {}

    [[nodiscard]] const std::string& pattern() const { return pattern_; }
    [[nodiscard]] const std::vector<std::string>& allowed_users() const { return allowed_users_; }
    [[nodiscard]] const std::vector<std::string>& allowed_roles() const { return allowed_roles_; }
    [[nodiscard]] const std::vector<std::string>& permissions() const { return permissions_; }

    [[nodiscard]] bool matches(const std::string& path) const { return std::regex_match(path, regex_); }
    [[nodiscard]] bool matches(const std::vector<std::string>& paths) const {
        bool found = true;
        for (const auto& path : paths) {
            found = found && std::regex_match(path, regex_);
        }
        return found;
    }
    [[nodiscard]] bool applies_to(const Identity& identity) const {
        return std::find(std::begin(allowed_users_), std::end(allowed_users_), identity.username()) !=
               std::end(allowed_users_);
    }
    [[nodiscard]] bool allows(const std::string& required) const {
        return std::find(std::begin(permissions_), std::end(permissions_), required) != std::end(permissions_);
    }

private:
    std::string pattern_;
    std::regex regex_;
    std::vector<std::string> allowed_users_;
    std::vector<std::string> allowed_roles_;
    std::vector<std::string> permissions_;
};

struct Rules
{
    std::vector<Rule> rules_;
};

struct Unrestricted
{
};

struct Permissions::Impl
{
    Impl() : permissions_(Unrestricted{}) {}
    explicit Impl(Rules&& rules) : permissions_(std::move(rules)) {}

    std::variant<Unrestricted, Rules> permissions_;
};

Permissions::Permissions() = default;

Permissions::Permissions(std::unique_ptr<Impl>&& impl) : impl_{std::move(impl)} {
}

Permissions::Permissions(Permissions&& rhs) noexcept : impl_{std::move(rhs.impl_)} {
}

Permissions::~Permissions() = default;

Permissions& Permissions::operator=(Permissions&& rhs) noexcept {
    if (this != &rhs) {
        impl_ = std::move(rhs.impl_);
    }
    return *this;
}

bool Permissions::good() const {
    return impl_ != nullptr;
}

bool Permissions::allows(const Identity& identity, const std::string& permission) const {
    return allows(identity, paths_t{ROOT}, permission);
}

bool Permissions::allows(const Identity& identity, const path_t& path, const std::string& permission) const {
    return allows(identity, paths_t{path}, permission);
}

bool Permissions::allows(const Identity& identity, const paths_t& paths, const std::string& permission) const {
    if (!good()) {
        // When no rules are loaded, we allow everything...
        // Dangerous, but backward compatible!
        return true;
    }

    bool allowed = false;
    std::visit(Overload{[&allowed](const Unrestricted&) {
                            // when no rules are loaded, we allow everything...
                            // Dangerous, but backward compatible!
                            allowed = true;
                        },
                        [&identity, &paths, &permission, &allowed](const Rules& rules) {
                            for (const auto& rule : rules.rules_) {
                                if (rule.matches(paths) && rule.applies_to(identity) && rule.allows(permission)) {
                                    allowed = true;
                                }
                            }
                        }},
               impl_->permissions_);

    return allowed;
}

Permissions::result_t Permissions::load_permissions_from_file(const fs::path& cfg) {
    using json = nlohmann::json;

    std::ifstream f(cfg.c_str());

    if (!f.good()) {
        return result_t::failure("Could not open file: " + cfg.string());
    }

    std::string l;
    std::getline(f, l);

    f.seekg(0, std::ios::beg);

    try {
        json data = json::parse(f);

        std::cout << "loaded data: " << data << std::endl;

        Rules rules;
        for (const auto& rule : data["rules"]) {
            std::string pattern = rule.contains("path") ? rule["path"] : "/.*";
            std::vector<std::string> allowed_users;
            if (rule.contains("allowed-users")) {
                allowed_users = rule["allowed-users"];
            }
            std::vector<std::string> allowed_roles;
            if (rule.contains("allowed-roles")) {
                allowed_users = rule["allowed-roles"];
            }
            std::vector<std::string> permissions;
            if (rule.contains("permissions")) {
                permissions = rule["permissions"];
            }
            rules.rules_.emplace_back(pattern, allowed_users, allowed_roles, permissions);
        }
        return result_t::success(Permissions(std::make_unique<Impl>(std::move(rules))));
    }
    catch (const json::parse_error& e) {
        return result_t::failure("Failed to load JSON: " + std::string(e.what()));
    }
}

} // namespace ecf
