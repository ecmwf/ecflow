/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_ApiV1_HPP
#define ecflow_http_ApiV1_HPP

#include "ecflow/http/DefsStorage.hpp"
#include "ecflow/http/HttpLibrary.hpp"

namespace ecf::http {

void routing(httplib::Server& http_server);

namespace v1 {

void not_implemented(const httplib::Request& request, httplib::Response& response);

void suites_options(const httplib::Request& request, httplib::Response& response);
void suites_create(const httplib::Request& request, httplib::Response& response);
void suites_read(const httplib::Request& request, httplib::Response& response);

void node_tree_options(const httplib::Request& request, httplib::Response& response);
void node_tree_read(const httplib::Request& request, httplib::Response& response);

void node_definition_options(const httplib::Request& request, httplib::Response& response);
void node_definition_read(const httplib::Request& request, httplib::Response& response);
void node_definition_update(const httplib::Request& request, httplib::Response& response);
void node_definition_delete(const httplib::Request& request, httplib::Response& response);

void node_status_options(const httplib::Request& request, httplib::Response& response);
void node_status_read(const httplib::Request& request, httplib::Response& response);
void node_status_update(const httplib::Request& request, httplib::Response& response);

void node_attributes_options(const httplib::Request& request, httplib::Response& response);
void node_attributes_create(const httplib::Request& request, httplib::Response& response);
void node_attributes_read(const httplib::Request& request, httplib::Response& response);
void node_attributes_update(const httplib::Request& request, httplib::Response& response);
void node_attributes_delete(const httplib::Request& request, httplib::Response& response);

void node_script_options(const httplib::Request& request, httplib::Response& response);
void node_script_read(const httplib::Request& request, httplib::Response& response);
void node_script_update(const httplib::Request& request, httplib::Response& response);

void node_output_options(const httplib::Request& request, httplib::Response& response);
void node_output_read(const httplib::Request& request, httplib::Response& response);

void server_ping_options(const httplib::Request& request, httplib::Response& response);
void server_ping_read(const httplib::Request& request, httplib::Response& response);

void server_status_options(const httplib::Request& request, httplib::Response& response);
void server_status_read(const httplib::Request& request, httplib::Response& response);
void server_status_update(const httplib::Request& request, httplib::Response& response);

void server_attributes_options(const httplib::Request& request, httplib::Response& response);
void server_attributes_create(const httplib::Request& request, httplib::Response& response);
void server_attributes_read(const httplib::Request& request, httplib::Response& response);
void server_attributes_update(const httplib::Request& request, httplib::Response& response);
void server_attributes_delete(const httplib::Request& request, httplib::Response& response);

void server_statistics_options(const httplib::Request& request, httplib::Response& response);
void server_statistics_read(const httplib::Request& request, httplib::Response& response);

} // namespace v1

} // namespace ecf::http

#endif /* ecflow_http_ApiV1_HPP */
