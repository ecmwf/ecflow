//============================================================================
// Name        : ssl_connection.cpp
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

#include "ssl_connection.hpp"

ssl_connection::~ssl_connection() {
#ifdef DEBUG_CONNECTION
   if (Ecf::server()) std::cout << "SERVER: ssl_connection::~ssl_connection socket_.is_open() = " << socket_.is_open() << "\n\n";
   else               std::cout << "CLIENT: ssl_connection::~ssl_connection socket_.is_open() = " << socket_.is_open() << "\n\n";
#endif
}


ssl_connection::ssl_connection(boost::asio::io_service& io_service , boost::asio::ssl::context& context)
: socket_(io_service,context)
{
#ifdef DEBUG_CONNECTION
   if (Ecf::server()) std::cout << "SERVER: ssl_connection::ssl_connection\n";
   else               std::cout << "CLIENT: ssl_connection::ssl_connection\n";
#endif
   socket_.set_verify_mode(boost::asio::ssl::verify_peer);

   // If we want server to verify client certificate, the use as below:
   // However this requires that certificate is installed.
   //if (Ecf::server())  socket_.set_verify_mode(boost::asio::ssl::verify_peer|boost::asio::ssl::verify_fail_if_no_peer_cert);
   //else                socket_.set_verify_mode(boost::asio::ssl::verify_peer);
   socket_.set_verify_callback(boost::bind(&ssl_connection::verify_certificate, this, _1, _2));
}

bool ssl_connection::verify_certificate(bool preverified,boost::asio::ssl::verify_context& ctx)
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

void ssl_connection::log_error(const char* msg)
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << in_context);
}

void ssl_connection::log_archive_error(const char* msg,const std::exception& ae,const std::string& data)
{
   const char* in_context = ", in client";
   if (Ecf::server()) in_context = ", in server";
   ecf::LogToCout logToCout;
   LOG(ecf::Log::ERR, msg << ae.what() << in_context << " data:\n" << data);
}


