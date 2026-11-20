// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

///
/// @brief Returns the copy of the tree of the ecFlow server, as last refreshed.
///
/// @return The copy, never null.
/// @throws HttpServerException with status 403 while the server uses Permissions (D6.6), and 503 when no copy is
///         available.
///
std::shared_ptr<Defs> get_defs();

///
/// @brief Returns whether the ecFlow server uses Permissions attributes, which the REST API does not support (D6.6).
///
/// The service learns it from the last refresh of its copy of the tree: either the refresh is denied by the
/// authorisation of Permissions (a server using them admits only the users it lists), or the refreshed tree carries a
/// server Permissions attribute. It holds until a refresh succeeds with a tree without a server attribute.
///
/// @return True while every request must be refused. Never throws.
///
bool server_uses_permissions() noexcept;

///
/// @brief Returns the message with which every request is refused while the server uses Permissions.
///
/// @return The message, stating that the REST API does not support Permissions.
///
const std::string& permissions_refusal();

void start_update_defs_loop(int interval);
void stop_update_defs_loop();

void trigger_defs_update();
void trigger_defs_update(std::function<void()> function);

} // namespace ecf::http
