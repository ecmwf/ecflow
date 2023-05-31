#ifndef APIV1IMPL_HPP
#define APIV1IMPL_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : ApiV1Impl
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

#ifdef ECF_OPENSSL
    #define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include "ClientInvoker.hpp"
#include "httplib.h"
#include "nlohmann/json.hpp"

void update_defs_loop(int interval);

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request);
std::unique_ptr<ClientInvoker> get_client(const nlohmann::json& j);

nlohmann::json get_sparser_node_tree(const std::string& path);

void add_suite(const httplib::Request& request, httplib::Response& response);

nlohmann::json get_suites();

nlohmann::json get_server_attributes();
nlohmann::json add_server_attribute(const httplib::Request& request);
nlohmann::json update_server_attribute(const httplib::Request& request);
nlohmann::json delete_server_attribute(const httplib::Request& request);

nlohmann::json get_node_definition(const std::string& path);
nlohmann::json update_node_definition(const httplib::Request& request);

nlohmann::json get_node_attributes(const std::string& path);
nlohmann::json add_node_attribute(const httplib::Request& request);
nlohmann::json update_node_attribute(const httplib::Request& request);
nlohmann::json delete_node_attribute(const httplib::Request& request);

nlohmann::json get_node_status(const httplib::Request& request);
nlohmann::json update_node_status(const httplib::Request& request);

nlohmann::json get_node_output(const httplib::Request& request);

nlohmann::json update_script_content(const httplib::Request& request);

#endif
