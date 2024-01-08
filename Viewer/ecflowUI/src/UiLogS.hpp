/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_UiLogS_HPP
#define ecflow_viewer_UiLogS_HPP

#include "UiLog.hpp"

class ServerHandler;

#define UI_FUNCTION_LOG_S(server) UiFunctionLogS __fclog(server, BOOST_CURRENT_FUNCTION);

class UiFunctionLogS : public UiFunctionLog {
public:
    UiFunctionLogS(ServerHandler* server, const std::string& funcName);
};

class UiLogS : public UiLog {
public:
    explicit UiLogS(ServerHandler* server);
};

#endif /* ecflow_viewer_UiLogS_HPP */
