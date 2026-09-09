/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_http_Client_HPP
#define ecflow_http_Client_HPP

#include <memory>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/HttpLibrary.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request);

std::unique_ptr<ClientInvoker> get_client_for_tasks(const httplib::Request& request, const ojson& payload);

} // namespace ecf::http

#endif /* ecflow_http_Client_HPP */
