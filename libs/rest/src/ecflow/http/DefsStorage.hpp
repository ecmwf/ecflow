/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
