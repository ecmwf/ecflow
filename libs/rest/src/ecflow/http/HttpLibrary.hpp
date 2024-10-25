/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_HttpLibrary_HPP
#define ecflow_http_HttpLibrary_HPP

#include <sys/socket.h>
#if defined(MSG_NOSIGNAL)
    // The MSG_NOSIGNAL flag should be defined by <sys/socket.h>
    // (as per POSIX: https://pubs.opengroup.org/onlinepubs/9699919799.2018edition/functions/send.html),
    // but that is not always the case for OSX.
    //
    // If MSG_NOSIGNAL is not defined, we leave it up to httplib to use default flags.
    //
    #define CPPHTTPLIB_SEND_FLAGS MSG_NOSIGNAL
    #define CPPHTTPLIB_RECV_FLAGS MSG_NOSIGNAL
#endif

#if defined(ECF_OPENSSL)
    #include <openssl/ssl.h>
    #if OPENSSL_VERSION_NUMBER < 0x1010100fL
        #warning OpenSSL versions prior to 1.1.1 detected. Aviso ETCD HTTP client will be build without OpenSSL support!
    #else
        #define CPPHTTPLIB_OPENSSL_SUPPORT
    #endif
#endif

#ifdef ECF_HTTP_COMPRESSION
    #define CPPHTTPLIB_ZLIB_SUPPORT
#endif

#include <httplib.h>

#endif /* ecflow_http_HttpLibrary_HPP */
