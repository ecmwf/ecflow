/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Permissions_HPP
#define ecflow_base_Permissions_HPP

#include <memory>
#include <regex>
#include <string>

#include "ecflow/base/Identity.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Result.hpp"

class ClientToServerCmd;

namespace ecf {

class Permissions {
    struct Impl;

    static constexpr auto ROOT = "/";

public:
    using result_t = Result<Permissions>;

    using path_t  = std::string;
    using paths_t = std::vector<std::string>;

    Permissions();

private:
    Permissions(std::unique_ptr<Impl>&& impl);

public:
    Permissions(const Permissions& rhs) = delete;
    Permissions(Permissions&& rhs) noexcept;

    Permissions& operator=(const Permissions& rhs) = delete;
    Permissions& operator=(Permissions&& rhs) noexcept;

    ~Permissions();

    [[nodiscard]] bool good() const;

    [[nodiscard]] bool authenticate(const Identity& identity) const {return true;};

    /**
     * Verify if the identity is allowed to perform the action on the give paths.
     *
     * @param identity the identity performing the action
     * @param path(s) the set of path(s) affected by the action
     * @param permission the required permission to perform the action
     * @return true if the identity is allowed to perform the action, false otherwise
     */
    [[nodiscard]] bool allows(const Identity& identity, const std::string& permission) const;
    [[nodiscard]] bool allows(const Identity& identity, const path_t& path, const std::string& permission) const;
    [[nodiscard]] bool allows(const Identity& identity, const paths_t& paths, const std::string& permission) const;

    [[nodiscard]] static result_t load_permissions_from_file(const fs::path& cfg);

private:
    std::unique_ptr<Impl> impl_;
};

} // namespace ecf

#endif // ecflow_base_Perms_HPP
