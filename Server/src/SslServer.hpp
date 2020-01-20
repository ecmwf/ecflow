#ifndef SSLSERVER_HPP_
#define SSLSERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : SslServer.cpp
// Author      : Avi
// Revision    : $Revision: #62 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : ECFLOW Server. Based on ASIO
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "BaseServer.hpp"
#include "SslTcpServer.hpp"

class SslServer : public BaseServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming connection.
   explicit SslServer(boost::asio::io_service& io_service,ServerEnvironment&);
   ~SslServer() override = default;

private:
   /// AbstractServer functions
   const std::string& ssl() const override;

   SslTcpServer server_;
};

#endif
