#ifndef SSYNC_CMD_HPP_
#define SSYNC_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #25 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>

#include "ServerToClientCmd.hpp"
#include "DefsDelta.hpp"
#include "DefsCache.hpp"

//================================================================================
// class SSyncCmd: Used to transfer changes made in the server to the client
//                 The client can then apply the changes to the client side defs.
//
// *** This class should be used in conjunction with the news command.
// *** i.e The news command is used to test for server changes. This command
// *** will then get those changes and merge them with client side defs, bringing
// *** client and server defs in sync.
//
// The *client_state_change_no* was passed from the client to the server
// The *client_modify_change_no* was passed from the client to the server
//
// This class make use of DefsCache as a performance optimisation.
//================================================================================
class SSyncCmd : public ServerToClientCmd {
public:
   // The constructor is *called* in the server.
   // This will collate the incremental changes made so far relative to the client_state_change_no.
   // For large scale change we use client_modify_change_no this will require a full update
   SSyncCmd(unsigned int client_handle,           // a reference to a set of suites used by client
            unsigned int client_state_change_no,
            unsigned int client_modify_change_no,
            AbstractServer* as);

   SSyncCmd() : ServerToClientCmd(), full_defs_(false), no_defs_(false), incremental_changes_(0) {}

   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual bool hasDefs() const { return true; }

   // Client side functions:
   virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;

   /// do_sync() is invoked on the *client side*, Can throw std::runtime_error
   /// Either does a *FULL* or *INCREMENTAL sync depending on the
   /// changes in the server. Returns true if client defs changed.
   bool do_sync( ServerReply& server_reply, bool debug = false) const;

private:

   friend class PreAllocatedReply;
   void init(unsigned int client_handle,          // a reference to a set of suites used by client
            unsigned int client_state_change_no,
            unsigned int client_modify_change_no,
            bool full_sync,
            AbstractServer* as);

   /// For use when doing a full sync
   void init(unsigned int client_handle,AbstractServer* as);

   void reset_data_members(unsigned int client_state_change_no);
   void full_sync(unsigned int client_handle,AbstractServer* as);

private:

   bool      full_defs_;
   bool      no_defs_;
   DefsDelta incremental_changes_;
   defs_ptr  server_defs_;         // for returning a subset of the suites
   std::string full_server_defs_as_string_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & no_defs_;                 // inform user defs does not exist.
      ar & full_defs_;               // returning full defs as a string
      ar & incremental_changes_;     // state changes, small scale changes

      /// When the server_defs_ was created the def's pointer on the suites was reset back to real server defs
      /// This is not correct for server_defs_, since we use the *same* suites
      /// **** This is OK since by default the Defs serialisation will fix up the suite's def's pointers ***
      /// The alternative is to clone all the suites, which is very expensive
      ar & server_defs_;  // large scale changes, if non zero handle, a small subset of the suites

      // when full_defs_ is set server_defs_ will be empty.
      if (Archive::is_saving::value) {
         // Avoid copying the string. As this could be very large  > 60MB
         if (full_defs_) {
            ar & DefsCache::full_server_defs_as_string_;
         }
         else ar & full_server_defs_as_string_;
      }
      else {
         ar & full_server_defs_as_string_;
      }
   }
};

std::ostream& operator<<(std::ostream& os, const SSyncCmd&);

#endif
