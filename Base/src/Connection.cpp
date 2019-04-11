//============================================================================
// Name        : Connection.cpp
// Author      : Avi
// Revision    : $Revision: #26 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : Serves as the connection between client server
//============================================================================

#include "Connection.hpp"

connection::~connection() {
#ifdef DEBUG_CONNECTION
   std::cout << "Connection::~connection  socket_.is_open() = " << socket_.is_open() << "\n\n";
#endif
}

connection::connection(boost::asio::io_service& io_service): socket_(io_service)
{
#ifdef DEBUG_CONNECTION
   std::cout << "Connection::connection\n";
#endif
}

void connection::log_error(const char* msg)
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << in_context);
}

void connection::log_archive_error(const char* msg,const std::exception& ae,const std::string& data)
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << ae.what() << in_context << " data:\n" << data);
}

