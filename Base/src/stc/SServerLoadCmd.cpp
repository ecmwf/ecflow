/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #3 $ 
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

#include <iostream>
#include "SServerLoadCmd.hpp"
#include "Gnuplot.hpp"

using namespace ecf;
using namespace std;
using namespace boost;

bool SServerLoadCmd::equals(ServerToClientCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<SServerLoadCmd*>(rhs);
   if (!the_rhs) return false;
   if (log_file_path_ != the_rhs->log_file_path()) return false;
   return ServerToClientCmd::equals(rhs);
}

std::ostream& SServerLoadCmd::print(std::ostream& os) const
{
   os << "cmd:SServerLoadCmd [ " << log_file_path_ << " ]";
   return os;
}

bool SServerLoadCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const
{
   if (debug) cout << "  SServerLoadCmd::handle_server_response log_file_path = " << log_file_path() << "\n";
   Gnuplot gnuplot(log_file_path(), server_reply.host(), server_reply.port());
   gnuplot.show_server_load();
   return true;
}

std::ostream& operator<<(std::ostream& os, const SServerLoadCmd& c)      { return c.print(os); }
