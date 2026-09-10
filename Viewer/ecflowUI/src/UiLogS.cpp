/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "UiLogS.hpp"

#include "ServerHandler.hpp"

//---------------------------------
// UiFunctionLog
//---------------------------------

UiFunctionLogS::UiFunctionLogS(ServerHandler* server, const std::string& funcName)
    : UiFunctionLog(((server) ? server->longName() : ""), funcName) {
}

//---------------------------------
// UiLog
//---------------------------------

UiLogS::UiLogS(ServerHandler* server)
    : UiLog((server) ? server->longName() : "") {
}
