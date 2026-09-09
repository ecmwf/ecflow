/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_http_DefsStorage_HPP
#define ecflow_http_DefsStorage_HPP

#include <functional>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

std::shared_ptr<Defs> get_defs();

void start_update_defs_loop(int interval);
void stop_update_defs_loop();

void trigger_defs_update();
void trigger_defs_update(std::function<void()> function);

} // namespace ecf::http

#endif /* DefsStorage */
