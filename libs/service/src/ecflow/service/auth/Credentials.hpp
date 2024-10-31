/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_auth_Credentials_HPP
#define ecflow_service_auth_Credentials_HPP

#include <optional>
#include <string>
#include <vector>

namespace ecf::service::auth {

class Credentials {
public:
    struct UserCredentials
    {
        std::string username;
        std::string password;
    };

    struct KeyCredentials
    {
        std::string email;
        std::string key;
    };

    Credentials() = default;

    void add(std::string key, std::string value);

    [[nodiscard]] std::optional<std::string> value(std::string_view key) const;

    [[nodiscard]] std::optional<UserCredentials> user() const;
    [[nodiscard]] std::optional<KeyCredentials> key() const;

    static Credentials load(const std::string& filepath);
    static Credentials load_content(const std::string& content);

private:
    struct Entry
    {
        std::string key;
        std::string value;
    };

    std::vector<Entry> entries_;
};

} // namespace ecf::service::auth

#endif /* ecflow_service_auth_Credentials_HPP */
