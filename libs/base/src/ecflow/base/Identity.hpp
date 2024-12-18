/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Identity_HPP
#define ecflow_base_Identity_HPP

#include <iostream>
#include <string>

namespace ecf {

class Identity {
public:
    Identity() : is_custom_{false}, username_{}, password_{} {}

    [[nodiscard]] static Identity make_user(const std::string& username) { return Identity{false, username, ""}; }
    [[nodiscard]] static Identity make_user(bool is_custom, const std::string& username, const std::string& password) {
        return Identity{is_custom, username, password};
    }

    [[nodiscard]] static Identity make_task() { return Identity{false, "task", ""}; }

    [[nodiscard]] bool is_custom() const { return is_custom_; }
    [[nodiscard]] const std::string& username() const { return username_; }
    [[nodiscard]] const std::string& password() const { return password_; }

private:
    Identity(bool is_custom, std::string username, std::string password)
        : is_custom_{is_custom},
          username_{std::move(username)},
          password_{std::move(password)} {}

    bool is_custom_;
    std::string username_;
    std::string password_;
};

} // namespace ecf

#endif // ecflow_base_Identity_HPP
