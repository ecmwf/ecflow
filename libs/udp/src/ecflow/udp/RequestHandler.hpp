/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_udp_RequestHandler_HPP
#define ecflow_udp_RequestHandler_HPP

#include <string>

namespace ecf {

/**
 * Enables the handling of all requests by a ecFlow UDP server
 */
struct RequestHandler
{
public:
    using inbound_t = std::string;

public:
    void handle(const inbound_t& request) const;
};

} // namespace ecf

#endif /* ecflow_udp_RequestHandler_HPP */
