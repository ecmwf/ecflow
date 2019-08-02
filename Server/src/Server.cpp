/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #173 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "Server.hpp"
#include "ServerEnvironment.hpp"

Server::Server(boost::asio::io_service& io_service, ServerEnvironment& serverEnv )
: BaseServer(io_service,serverEnv),
  tcp_server_(this,io_service,serverEnv)
{
}
