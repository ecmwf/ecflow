/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/User.hpp"

#include <cassert>
#include <cerrno>  // for errno()
#include <cstring> // for strerror()
#include <pwd.h>   // for getpwuid()
#include <sstream>
#include <stdexcept>
#include <unistd.h> // ofr getuid()

#include "Message.hpp"

namespace ecf {

std::string get_login_name() {
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

            throw std::runtime_error(
                MESSAGE("UserCmd::get_user: could not determine user name for uid " << real_user_id_of_process));
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
