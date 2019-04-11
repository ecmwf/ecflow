#ifndef OPENSSL_HPP_
#define OPENSSL_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include <boost/asio/ssl.hpp>
#include <string>

namespace ecf {

class Openssl : private boost::noncopyable {
public:

   /// Directory where ssl certificates are held
   static std::string certificates_dir();

   /// There is no SSL protocol version named SSLv23.
   /// The SSLv23_method() API and its variants choose SSLv2, SSLv3, or TLSv1 for compatibility with the peer
   /// Consider the incompatibility among the SSL/TLS versions when you develop
   /// SSL client/server applications. For example, a TLSv1 server cannot understand
   /// a client-hello message from an SSLv2 or SSLv3 client.
   /// The SSLv2 client/server recognises messages from only an SSLv2 peer.
   /// The SSLv23_method() API and its variants may be used when the compatibility with the peer is important.
   /// An SSL server with the SSLv23 method can understand any of the SSLv2, SSLv3, and TLSv1 hello messages.
   /// However, the SSL client using the SSLv23 method cannot establish connection with the SSL server
   ///  with the SSLv3/TLSv1 method because SSLv2 hello message is sent by the client
   static boost::asio::ssl::context::method method() { return boost::asio::ssl::context::sslv23;}

   /// Check the right files exist
   static void check_server_certificates();
   static void check_client_certificates();
   static const char* ssl_info();

private:
   Openssl();
};

}

#endif
