/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/User.hpp"

#include <cassert>
#include <cerrno>  // for errno()
#include <cstring> // for strerror()
#include <pwd.h>   // for getpwuid()
#include <sstream>
#include <stdexcept>
#include <unistd.h> // ofr getuid()

#include "ecflow/core/Enumerate.hpp"

namespace ecf {

namespace detail {

template <>
struct EnumTraits<User::Action>
{
    using underlying_t = std::underlying_type_t<User::Action>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(User::Action::FOB, "fob"),
        std::make_pair(User::Action::FAIL, "fail"),
        std::make_pair(User::Action::ADOPT, "adopt"),
        std::make_pair(User::Action::REMOVE, "remove"),
        std::make_pair(User::Action::BLOCK, "block"),
        std::make_pair(User::Action::KILL, "kill"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<User::Action>::size == map.back().first + 1);
};

} // namespace detail

bool User::valid_user_action(const std::string& s) {
    return Enumerate<User::Action>::is_valid(s);
}

User::Action User::user_action(const std::string& s) {
    return Enumerate<User::Action>::to_enum(s).value_or(User::BLOCK);
}

std::string User::to_string(User::Action uc) {
    if (auto found = Enumerate<User::Action>::to_string(uc); found) {
        return std::string{found.value()};
    }
    assert(false);
    return std::string();
}

std::string User::login_name() {
    static std::string the_user_name;
    if (the_user_name.empty()) {

        // Get the uid of the running process and use it to get a record from /etc/passwd */
        // getuid() cannot fail, but getpwuid can fail.
        errno                         = 0;
        uid_t real_user_id_of_process = getuid();
        struct passwd* thePassWord    = getpwuid(real_user_id_of_process);
        if (thePassWord == nullptr) {
            if (errno != 0) {
                std::string theError = strerror(errno);
                throw std::runtime_error("UserCmd::get_user: could not determine user name. Because: " + theError);
            }

            std::stringstream ss;
            ss << "UserCmd::get_user: could not determine user name for uid " << real_user_id_of_process;
            throw std::runtime_error(ss.str());
        }

        the_user_name = thePassWord->pw_name; // equivalent to the login name
        if (the_user_name.empty()) {
            throw std::runtime_error(
                "UserCmd::get_user: could not determine user name. Because: thePassWord->pw_name is empty");
        }
    }
    return the_user_name;
}

} // namespace ecf
