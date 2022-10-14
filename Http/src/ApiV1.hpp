#ifndef APIV1_HPP
#define APIV1_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ApiV1
// Author      : partio
// Revision    : $Revision$ 
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#define CPPHTTPLIB_SEND_FLAGS MSG_NOSIGNAL
#define CPPHTTPLIB_RECV_FLAGS MSG_NOSIGNAL

#ifdef ECF_OPENSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <httplib.h>

class ApiV1 {
  public:
   static void create(httplib::Server& server);
};

#endif
