/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/Api.hpp"

#include "ecflow/http/ApiV1.hpp"
#include "ecflow/http/DefsStorage.hpp"
#include "ecflow/http/Options.hpp"

namespace ecf::http {

void setup(httplib::Server& http_server) {
    routing(http_server);
    update_defs_loop(opts.polling_interval);

    if (opts.verbose) {
        printf("API v1 ready\n");
    }
}

} // namespace ecf::http
