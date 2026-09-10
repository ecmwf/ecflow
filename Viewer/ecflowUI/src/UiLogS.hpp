/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_UiLogS_HPP
#define ecflow_viewer_UiLogS_HPP

#include "UiLog.hpp"

class ServerHandler;

#define UI_FUNCTION_LOG_S(server) UiFunctionLogS __fclog(server, __func__);

class UiFunctionLogS : public UiFunctionLog {
public:
    UiFunctionLogS(ServerHandler* server, const std::string& funcName);
};

class UiLogS : public UiLog {
public:
    explicit UiLogS(ServerHandler* server);
};

#endif /* ecflow_viewer_UiLogS_HPP */
