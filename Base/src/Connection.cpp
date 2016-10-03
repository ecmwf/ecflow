//============================================================================
// Name        : Connection.cpp
// Author      : Avi
// Revision    : $Revision: #26 $
//
// Copyright 2009-2016 ECMWF.
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

#ifdef ECF_OPENSSL
connection::connection(boost::asio::io_service& io_service , boost::asio::ssl::context& context)
: allow_new_client_old_server_(0),
  allow_old_client_new_server_(0),
  socket_(io_service,context)
{
#ifdef DEBUG_CONNECTION
   std::cout << "Connection::connection openssl\n";
#endif
   socket_.set_verify_mode(boost::asio::ssl::verify_peer);

   // If we want server to verify client certificate, the use as below:
   // However this requires that certificate is installed.
   //if (Ecf::server())  socket_.set_verify_mode(boost::asio::ssl::verify_peer|boost::asio::ssl::verify_fail_if_no_peer_cert);
   //else                socket_.set_verify_mode(boost::asio::ssl::verify_peer);
   socket_.set_verify_callback(boost::bind(&connection::verify_certificate, this, _1, _2));
}

bool connection::verify_certificate(bool preverified,boost::asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    // std::cout << "Verifying " << subject_name << "\n";

    return preverified;
}
#else
connection::connection(boost::asio::io_service& io_service)
: allow_new_client_old_server_(0),
  allow_old_client_new_server_(0),
  socket_(io_service)
{
#ifdef DEBUG_CONNECTION
   std::cout << "Connection::connection\n";
#endif
}
#endif


void connection::log_error(const char* msg) const
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << in_context);
}

void connection::log_archive_error(const char* msg,const boost::archive::archive_exception& ae) const
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << ae.what() << in_context);
}
