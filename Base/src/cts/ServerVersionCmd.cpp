/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : returns the server version.
//               This command should not be changed
//                It will allow new clients to ask OLD server their version numbers
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Version.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

std::ostream& ServerVersionCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::server_version());
}

bool ServerVersionCmd::equals(ClientToServerCmd* rhs) const
{
   ServerVersionCmd* the_rhs = dynamic_cast< ServerVersionCmd* > ( rhs );
   if ( !the_rhs ) return false;
   return UserCmd::equals(rhs);
}

const char* ServerVersionCmd::theArg() const
{
   return CtsApi::server_version_arg();
}

STC_Cmd_ptr ServerVersionCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().server_version_++;
   return PreAllocatedReply::string_cmd(Version::raw());
}

static const char* arg_desc()
{
            /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return
            "Returns the version number of the server\n"
            "Usage:\n"
            "  --server_version\n"
            "    Writes the version to standard output\n"
            ;
}

void ServerVersionCmd::addOption(boost::program_options::options_description& desc) const
{
   desc.add_options()(CtsApi::server_version_arg(),arg_desc());
}

void ServerVersionCmd::create(    Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv*  ace ) const
{
   if (ace->debug()) cout << "  ServerVersionCmd::create\n";

   // testing client interface
   if (ace->under_test())  return;

   cmd = Cmd_ptr(new ServerVersionCmd());
}

std::ostream& operator<<(std::ostream& os, const ServerVersionCmd& c) { return c.print(os); }
