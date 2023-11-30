/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_User_HPP
#define ecflow_core_User_HPP

#include <string>

namespace ecf {

class User {
public:
    enum Action { FOB, FAIL, ADOPT, REMOVE, BLOCK, KILL };

    static bool valid_user_action(const std::string&);
    static Action user_action(const std::string&);
    static std::string to_string(Action);

    // return login name: will throw if there are any errors
    static std::string login_name();

private:
    User(const User&)                  = delete;
    const User& operator=(const User&) = delete;
    User()                             = delete;
};

} // namespace ecf

#endif /* ecflow_core_User_HPP */
