/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_ApiV1Impl_HPP
#define ecflow_http_ApiV1Impl_HPP

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
#include "JSON.hpp"
#include "httplib.h"

void update_defs_loop(int interval);

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request);
std::unique_ptr<ClientInvoker> get_client(const ecf::ojson& j);

ecf::ojson get_sparser_node_tree(const std::string& path);

void add_suite(const httplib::Request& request, httplib::Response& response);

ecf::ojson get_suites();

ecf::ojson get_server_attributes();
ecf::ojson add_server_attribute(const httplib::Request& request);
ecf::ojson update_server_attribute(const httplib::Request& request);
ecf::ojson delete_server_attribute(const httplib::Request& request);

ecf::ojson get_node_definition(const std::string& path);
ecf::ojson update_node_definition(const httplib::Request& request);

ecf::ojson get_node_attributes(const std::string& path);
ecf::ojson add_node_attribute(const httplib::Request& request);
ecf::ojson update_node_attribute(const httplib::Request& request);
ecf::ojson delete_node_attribute(const httplib::Request& request);

ecf::ojson get_node_status(const httplib::Request& request);
ecf::ojson update_node_status(const httplib::Request& request);

ecf::ojson get_node_output(const httplib::Request& request);

ecf::ojson update_script_content(const httplib::Request& request);

#endif /* ecflow_http_ApiV1Impl_HPP */
