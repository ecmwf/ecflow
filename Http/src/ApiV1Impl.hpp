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

#define CPPHTTPLIB_SEND_FLAGS MSG_NOSIGNAL
#define CPPHTTPLIB_RECV_FLAGS MSG_NOSIGNAL

#ifdef ECF_OPENSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <nlohmann/json.hpp>
#include <httplib.h>
#include "ClientInvoker.hpp"

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
