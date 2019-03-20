/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <assert.h>
#include <stdexcept>
#include <unistd.h>     // ofr getuid()
#include <pwd.h>        // for getpwuid()
#include <string.h>     // for strerror()
#include <errno.h>      // for errno()
#include <sstream>

#include "User.hpp"

namespace ecf {

bool User::valid_user_action( const std::string& s)
{
   if (s == "fob") return true;
   if (s == "fail") return true;
   if (s == "adopt") return true;
   if (s == "remove") return true;
   if (s == "block") return true;
   if (s == "kill") return true;
   return false;
}

User::Action User::user_action( const std::string& s)
{
   if (s == "fob") return User::FOB;
   if (s == "fail") return User::FAIL;
   if (s == "adopt") return User::ADOPT;
   if (s == "remove") return User::REMOVE;
   if (s == "block") return User::BLOCK;
   if (s == "kill") return User::KILL;
   return User::BLOCK;
}

std::string User::to_string(User::Action uc)
{
   switch (uc) {
      case User::FOB:  return "fob"; break;
      case User::FAIL: return "fail"; break;
      case User::ADOPT: return "adopt"; break;
      case User::REMOVE: return "remove"; break;
      case User::BLOCK: return "block"; break;
      case User::KILL: return "kill"; break;
   }
   assert(false);
   return std::string();
}

std::string User::login_name()
{
   static std::string the_user_name;
   if (the_user_name.empty()) {

      // Get the uid of the running process and use it to get a record from /etc/passwd */
      // getuid() can not fail, but getpwuid can fail.
      errno = 0;
      uid_t real_user_id_of_process = getuid();
      struct passwd * thePassWord = getpwuid ( real_user_id_of_process );
      if (thePassWord == 0 ) {
         if ( errno != 0) {
            std::string theError = strerror(errno);
            throw std::runtime_error("UserCmd::get_user: could not determine user name. Because: " + theError);
         }

         std::stringstream ss;
         ss << "UserCmd::get_user: could not determine user name for uid " << real_user_id_of_process;
         throw std::runtime_error(ss.str());
      }

      the_user_name = thePassWord->pw_name;  // equivalent to the login name
      if ( the_user_name.empty() ) {
         throw std::runtime_error("UserCmd::get_user: could not determine user name. Because: thePassWord->pw_name is empty");
      }
   }
   return the_user_name;
}

}
