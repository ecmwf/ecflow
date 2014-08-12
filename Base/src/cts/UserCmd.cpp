/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #65 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <sstream>
#include <fstream>

#include <pwd.h>       /* getpwuid */
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>      /* tolower */

#include "ClientToServerCmd.hpp"

#include "AbstractServer.hpp"
#include "Log.hpp"
#include "Str.hpp"

using namespace std;
using namespace boost;
using namespace ecf;

bool UserCmd::equals(ClientToServerCmd* rhs) const
{
   UserCmd* the_rhs = dynamic_cast< UserCmd* > ( rhs );
   if ( !the_rhs ) return false;
   return user_ == the_rhs->user();
}

bool UserCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& ) const
{
   LOG_ASSERT(!user().empty(),"");

   // get the current user. This should have been set by the client
   if (as->authenticateUser(user())) {
      if (as->authenticateWriteAccess(user(),isWrite())) {
         return true;
      }

      std::string msg = "[ authentication failed ] User ";
      msg += user();
      msg += " has no write access. Please see your administrator.";
      throw std::runtime_error( msg );
   }

   std::string msg = "[ authentication failed ] User ";
   msg += user();
   msg += " is not allowed any access.";
   throw std::runtime_error( msg );

   return false;
}

void UserCmd::setup_user_authentification()
{
   // Minimise system calls by using static.
   static std::string the_user_name;
   if (the_user_name.empty()) {
      // Get the uid of the running process and use it to get a record from /etc/passwd */
      struct passwd * thePassWord = getpwuid ( getuid() );
      the_user_name = thePassWord->pw_name;  // equivalent to the login name
   }

   user_ = the_user_name;
}

void UserCmd::prompt_for_confirmation(const std::string& prompt) const
{
   cout << prompt;
   char reply[256];
   cin.getline (reply,256);
   if (reply[0] != 'y' && reply[0] != 'Y') {
      exit(1);
   }
}

std::ostream& UserCmd::user_cmd(std::ostream& os, const std::string& the_cmd) const
{
   return os << the_cmd << " :" << user();
}

//#define DEBUG_ME 1

void UserCmd::split_args_to_options_and_paths(
          const std::vector<std::string>& args,
          std::vector<std::string>& options,
          std::vector<std::string>& paths)
{
   size_t vec_size = args.size();
   for(size_t i = 0; i < vec_size; i++) {
      if (args[i].empty()) continue;
      if (args[i][0] == '/') paths.push_back(args[i]);
      else                   options.push_back(args[i]);
   }

#ifdef DEBUG_ME
   std::cout << "split_args_to_options_and_paths\n";
   for(size_t i = 0; i < args.size(); ++i) { std::cout << "args[" << i << "]=" <<args[i] << "\n"; }
   for(size_t i = 0; i < options.size(); ++i) { std::cout << "options[" << i << "]=" << options[i] << "\n"; }
   for(size_t i = 0; i < paths.size(); ++i) { std::cout << "paths[" << i << "]=" << paths[i] << "\n"; }
#endif
}

int UserCmd::time_out_for_load_sync_and_get() const { return 600; }

