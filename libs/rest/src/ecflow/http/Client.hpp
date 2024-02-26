/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_Client_HPP
#define ecflow_http_Client_HPP

#include <memory>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/HttpLibrary.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request);

std::unique_ptr<ClientInvoker> get_client_for_tasks(const httplib::Request& request, const ojson& payload);

} // namespace ecf::http

#endif /* ecflow_http_Client_HPP */
