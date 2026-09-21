// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/Defs.hpp"

class ServerHandler;

// -------------------------------------------------------------------------
// ServerDefsAccess - a class to manage access to the server definition tree
// - required for multi-threaded access
// -------------------------------------------------------------------------

class ServerDefsAccess {
public:
    explicit ServerDefsAccess(ServerHandler* server);
    ~ServerDefsAccess();

    defs_ptr defs();

private:
    ServerHandler* server_;
};
