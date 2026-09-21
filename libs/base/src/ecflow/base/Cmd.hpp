// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

class ClientToServerCmd;
class ServerToClientCmd;
class ServerReply;

using Cmd_ptr     = std::shared_ptr<ClientToServerCmd>;
using STC_Cmd_ptr = std::shared_ptr<ServerToClientCmd>;
