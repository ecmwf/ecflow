/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_service_auth_Credentials_HPP
#define ecflow_service_auth_Credentials_HPP

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ecf::service::auth {

class Credentials {
public:
    struct Error
    {
        std::string message;
        explicit Error(std::string message)
            : message(std::move(message)) {}
    };

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

    using expected_t = std::variant<Credentials, Error>;

    static expected_t load(const std::string& filepath);
    static expected_t load_content(const std::string& content);

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
