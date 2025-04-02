/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_AuthorisationService_HPP
#define ecflow_server_AuthorisationService_HPP

#include <string>
#include <vector>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Result.hpp"
#include "ecflow/node/Permissions.hpp"

class AbstractServer;

namespace ecf {

class AuthorisationService {
public:
    static constexpr auto ROOT = "/";

    using result_t = Result<AuthorisationService>;
    using path_t   = std::string;
    using paths_t  = std::vector<std::string>;

    AuthorisationService();

    AuthorisationService(const AuthorisationService& rhs) = delete;
    AuthorisationService(AuthorisationService&& rhs) noexcept;

    AuthorisationService& operator=(const AuthorisationService& rhs) = delete;
    AuthorisationService& operator=(AuthorisationService&& rhs) noexcept;

    ~AuthorisationService();

    [[nodiscard]]
    bool good() const;

    [[nodiscard]]
    bool authenticate(const Identity& identity) const {
        return true;
    }

    /**
     * Verify if the identity is allowed to perform the action on the give paths.
     *
     * @param identity the identity performing the action
     * @param paths the set of path(s) affected by the action
     * @param permission the required permission to perform the action
     * @return true if the identity is allowed to perform the action, false otherwise
     */
    [[nodiscard]]
    bool allows(const Identity& identity, const AbstractServer& server, Allowed required) const;

    [[nodiscard]]
    bool allows(const Identity& identity, const AbstractServer& server, const path_t& path, Allowed required) const;

    [[nodiscard]]
    bool allows(const Identity& identity, const AbstractServer& server, const paths_t& paths, Allowed required) const;

    [[nodiscard]]
    static result_t load_permissions_from_nodes();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    AuthorisationService(std::unique_ptr<Impl>&& impl);
};

} // namespace ecf

#endif /* ecflow_server_AuthorisationService_HPP */
