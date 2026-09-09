/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ServerDefsAccess.hpp"

#include "ServerHandler.hpp"

ServerDefsAccess::ServerDefsAccess(ServerHandler* server)
    : server_(server) {
    server_->defsMutex_.lock(); // lock the resource on construction
}

ServerDefsAccess::~ServerDefsAccess() {
    server_->defsMutex_.unlock(); // unlock the resource on destruction
}

defs_ptr ServerDefsAccess::defs() {
    return server_->defs(); // the resource will always be locked when we use it
}
