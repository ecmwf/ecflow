#ifndef CLIENT_TO_SERVER_REQUEST_HPP_
#define CLIENT_TO_SERVER_REQUEST_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include <boost/noncopyable.hpp>
#include <boost/serialization/tracking.hpp>
#include "ClientToServerCmd.hpp"

// Base class for client to server requesting.
// This class is used in the IPC messaging from  client to server.
class ClientToServerRequest : private boost::noncopyable {
public:

   ClientToServerRequest() {}
   ClientToServerRequest(const Cmd_ptr& cmd) : cmd_(cmd) { cmd_->setup_user_authentification();}
   ~ClientToServerRequest() {}

   void set_cmd(const Cmd_ptr& cmd) { cmd_ = cmd; cmd_->setup_user_authentification(); }
   Cmd_ptr get_cmd() const { return cmd_;}

   /// This is called in the server only, to handle the quest.
   STC_Cmd_ptr handleRequest(AbstractServer*) const;

   std::ostream& print(std::ostream& os) const;

   bool getRequest()  const { return (cmd_.get()) ? cmd_->get_cmd()  : false; }
   bool terminateRequest() const { return (cmd_.get()) ? cmd_->terminate_cmd() : false;  }
   bool groupRequest() const { return (cmd_.get()) ? cmd_->group_cmd() : false;  }

   /// Used by boost test, to verify persistence
   bool operator==(const ClientToServerRequest& rhs) const;

private:
   Cmd_ptr cmd_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & cmd_;
   }
};

std::ostream& operator<<(std::ostream& os, const ClientToServerRequest& d);

// Do NOT use
//    BOOST_CLASS_IMPLEMENTATION(ClientToServerRequest, boost::serialization::object_serializable)
//    i.e eliminate serialisation overhead at the cost of never being able to increase the version.
// Since we may need use version ing in the future

// This should ONLY be added to objects that are *NOT* serialised through a pointer
//   Eliminate object tracking (even if serialised through a pointer)
//   at the risk of a programming error creating duplicate objects.
BOOST_CLASS_TRACKING(ClientToServerRequest,boost::serialization::track_never);

#endif
