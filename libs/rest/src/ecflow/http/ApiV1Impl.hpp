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

#include "ecflow/core/HttpLibrary.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

ojson get_basic_node_tree(const std::string& path);
ojson get_full_node_tree(const std::string& path, bool with_id, bool with_gen_vars);
ojson get_sparser_node_tree(const std::string& path);

void add_suite(const httplib::Request& request, httplib::Response& response);

ojson get_suites();

ojson get_server_attributes();
ojson add_server_attribute(const httplib::Request& request);
ojson update_server_attribute(const httplib::Request& request);
ojson delete_server_attribute(const httplib::Request& request);

ojson get_node_definition(const std::string& path);
ojson update_node_definition(const httplib::Request& request);

ojson get_node_attributes(const std::string& path);
ojson add_node_attribute(const httplib::Request& request);
ojson update_node_attribute(const httplib::Request& request);
ojson delete_node_attribute(const httplib::Request& request);

ojson get_node_status(const httplib::Request& request);
ojson update_node_status(const httplib::Request& request);

ojson get_node_output(const httplib::Request& request);

ojson update_script_content(const httplib::Request& request);

} // namespace ecf::http

#endif /* ecflow_http_ApiV1Impl_HPP */
