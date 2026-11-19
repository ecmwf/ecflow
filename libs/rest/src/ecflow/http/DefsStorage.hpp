// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <functional>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/http/JSON.hpp"

namespace ecf::http {

///
/// @brief Computes the interval between two polls of the ecFlow server, lengthened while the REST API is idle.
///
/// The interval grows by one second for each minute without a request, up to the maximum, which is never below thirty
/// seconds. The growth is disabled when the maximum is not above the base interval.
///
/// @param[in] base The base interval, in seconds.
/// @param[in] max_option The maximum interval, in seconds, as configured.
/// @param[in] last_request_age The time since the last request, in seconds.
/// @return The interval to wait before the next poll.
///
std::chrono::seconds polling_interval(int base, int max_option, double last_request_age);

std::shared_ptr<Defs> get_defs();

void start_update_defs_loop(int interval);
void stop_update_defs_loop();

void trigger_defs_update();
void trigger_defs_update(std::function<void()> function);

} // namespace ecf::http
