/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $
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
#include "SClientHandleSuitesCmd.hpp"
#include "AbstractServer.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "ClientSuiteMgr.hpp"
#include "ClientToServerCmd.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

SClientHandleSuitesCmd::SClientHandleSuitesCmd(AbstractServer* as )
{
   init(as);
}

void SClientHandleSuitesCmd::init(AbstractServer* as)
{
   // This command can be re-used hence clear existing data members
   users_.clear();
   client_handles_.clear();

   ClientSuiteMgr& client_suite_mgr = as->defs()->client_suite_mgr();
   const std::vector<ecf::ClientSuites>& clientSuites = client_suite_mgr.clientSuites();

   size_t client_suites_size = clientSuites.size();
   client_handles_.reserve(client_suites_size);
   for(size_t c = 0; c < client_suites_size; c++) {

      // The handle suites are already ordered same as Defs suites
      std::vector<std::string> suite_names;
      clientSuites[c].suites( suite_names );

      client_handles_.emplace_back(clientSuites[c].handle(), suite_names );

      // Create user, and his list of handles
      bool fnd_user = false;
      for(auto & user : users_) {
         if (user.first == clientSuites[c].user()) {
            user.second.push_back(clientSuites[c].handle());
            fnd_user = true;
            break;
         }
      }
      if (!fnd_user) {
         std::vector<unsigned int> handles;  handles.push_back(clientSuites[c].handle());
         users_.emplace_back(clientSuites[c].user(),handles );
      }
   }
}

bool SClientHandleSuitesCmd::equals(ServerToClientCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<SClientHandleSuitesCmd*>(rhs);
   if (!the_rhs) return false;
   return ServerToClientCmd::equals(rhs);
}

std::ostream& SClientHandleSuitesCmd::print(std::ostream& os) const
{
   os << "cmd:SClientHandleSuitesCmd ";
   return os;
}

bool SClientHandleSuitesCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
   if (debug) std::cout << "  SClientHandleSuitesCmd::handle_server_response\n";

   if (server_reply.cli() && !cts_cmd->group_cmd()) {
      /// This Could be part of a group command, hence ONLY if NOT group command

      // print out:
      // user  handle suites
      // user1 1 s1 s2 s3
      //       2 s1 s2
      // user2 1 s1 s2 s3
      //       2 s1 s2
      for(size_t u = 0; u < users_.size(); u++) {
         if (u == 0) {
            cout << "\n";
            cout << left << setw(10) << "User" << setw(6) << "handle" << "  suites\n";
         }
         cout << left << setw(10) << users_[u].first;
         for(size_t h = 0; h < users_[u].second.size(); h++) {
            unsigned int handle = users_[u].second[h];
            for(const auto & client_handle : client_handles_) {
               if (handle == client_handle.first) {
                  if (h != 0) cout << "          "; // 10 spaces to align handles
                  cout << right << setw(6) << handle << "  ";
                  const std::vector<std::string>& suites = client_handle.second;
                  for(const auto & suite : suites) { cout << suite << "  "; }
                  cout << "\n";
               }
            }
         }
      }
   }
   else {
      server_reply.set_client_handle_suites(client_handles_);
   }
   return true;
}

std::ostream& operator<<(std::ostream& os, const SClientHandleSuitesCmd& c) { return c.print(os); }
