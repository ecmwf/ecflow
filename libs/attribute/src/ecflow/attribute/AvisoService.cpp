/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/AvisoService.hpp"

#include <fstream>
#include <iostream>

#include "ecflow/core/Filesystem.hpp"
#include "nlohmann/json.hpp"

namespace ecf {

void AvisoAPI::check_aviso_server_for_notifications(AvisoRegistry& registry) const {
    std::lock_guard guard(m_);

    using json = nlohmann::ordered_json;

    // TODO[MB]: Actually implement retrieval of notification from Aviso server

    fs::path f = "notification_api.json";
    boost::system::error_code ec;
    if (fs::exists(f, ec)) {

        json object;
        {
            std::ifstream ifs("notification_api.json");
            object = json::parse(ifs);
        }

        fs::remove(f);

        for (const auto& handle : object["handlers"]) {
            auto name = handle["name"];
            registry.notify(std::string{name});
        }
    }
}

} // namespace ecf
