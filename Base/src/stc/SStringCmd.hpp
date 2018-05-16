#ifndef SSTRING_CMD_HPP_
#define SSTRING_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
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

#include "ServerToClientCmd.hpp"

///================================================================================
/// Paired with CFileCmd
/// Client---(CFileCmd)---->Server-----(SStringCmd)--->client:
/// Only valid when the clients request a CFileCmd *OR* Log file
/// Other times this will be empty.
/// CFileCmd:: The file Contents(script,job,jobout,manual)
/// 	Can be potentially very large
/// LogCmd: The log file Can be potentially very large
///     Only really valid if the out bound request was a LogCmd(LogCmd::GET)
///================================================================================
class SStringCmd : public ServerToClientCmd {
public:
   explicit SStringCmd(const std::string& s) : str_(s) {}
   SStringCmd() : ServerToClientCmd() {}

   void init(const std::string& s) { str_ = s;}
   virtual std::ostream& print(std::ostream& os) const;
   virtual bool equals(ServerToClientCmd*) const;
   virtual const std::string& get_string() const { return str_;} // used by group command
   virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;
   virtual void cleanup() { std::string().swap(str_);} /// run in the server, after command send to client

private:
   std::string str_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize( Archive & ar, const unsigned int /*version*/ ) {
      ar & boost::serialization::base_object< ServerToClientCmd >( *this );
      ar & str_;
   }
};

std::ostream& operator<<(std::ostream& os, const SStringCmd&);

#endif
