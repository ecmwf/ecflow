#ifndef SERVERTOCLIENTREQUEST_HPP_
#define SERVERTOCLIENTREQUEST_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #30 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include "ServerToClientCmd.hpp"

// Base class for server to client requesting. This class is used in the IPC messaging between
// server and client
class ServerToClientResponse : private boost::noncopyable {
public:
   ServerToClientResponse() {}
   ServerToClientResponse(const STC_Cmd_ptr& cmd) : stc_cmd_(cmd) {}
   ~ServerToClientResponse() {}

   STC_Cmd_ptr get_cmd() const { return stc_cmd_; }
   void set_cmd(const STC_Cmd_ptr& cmd) { stc_cmd_ = cmd;}

   std::ostream& print(std::ostream& os) const;

   /// Handle the response from the server. On the client side
   /// return true IF and ONLY IF client response was ok, if further client action required return false
   bool handle_server_response( ServerReply&, Cmd_ptr cts_cmd, bool debug ) const;

   /// Used by boost test, to verify persistence
   bool operator==(const ServerToClientResponse& rhs) const;

private:
   STC_Cmd_ptr stc_cmd_;
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & stc_cmd_;
   }
};

std::ostream& operator<<(std::ostream& os, const ServerToClientResponse& d);

// Do NOT use
//    BOOST_CLASS_IMPLEMENTATION(ecf::ServerToClientResponse, boost::serialization::object_serializable)
//    i.e eliminate serialisation overhead at the cost of never being able to increase the version.
// Since we may need use version ing in the future

// This should ONLY be added to objects that are *NOT* serialised through a pointer
//   Eliminate object tracking (even if serialised through a pointer)
//   at the risk of a programming error creating duplicate objects.
BOOST_CLASS_TRACKING(ServerToClientResponse,boost::serialization::track_never);

#endif
