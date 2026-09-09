/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/http/Api.hpp"

#include "ecflow/http/ApiV1.hpp"
#include "ecflow/http/DefsStorage.hpp"
#include "ecflow/http/Options.hpp"

namespace ecf::http {

void setup(httplib::Server& http_server) {
    routing(http_server);
    start_update_defs_loop(opts.polling_interval);

    if (opts.verbose) {
        printf("API v1 ready\n");
    }
}

void teardown() {
    stop_update_defs_loop();
}

} // namespace ecf::http
