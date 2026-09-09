/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_Cmd_HPP
#define ecflow_base_Cmd_HPP

#include <memory>

class ClientToServerCmd;
class ServerToClientCmd;
class ServerReply;

using Cmd_ptr     = std::shared_ptr<ClientToServerCmd>;
using STC_Cmd_ptr = std::shared_ptr<ServerToClientCmd>;

#endif /* ecflow_base_Cmd_HPP */
