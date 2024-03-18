/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "aviso/ListenerSchema.hpp"

#include <fstream>
#include <istream>

#include <nlohmann/json.hpp>

namespace aviso {

ListenerSchema ListenerSchema::load(const std::string& schema_path) {
    std::ifstream schema_stream(schema_path);
    return load(schema_stream);
}

ListenerSchema ListenerSchema::load(std::istream& schema_stream) {
    using json = nlohmann::ordered_json;

    json data;
    try {
        data = json::parse(schema_stream);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse listener schema: " + std::string(e.what()));
    }

    ListenerSchema schema{};

    for (json::iterator entry = data.begin(); entry != data.end(); ++entry) {
        if (auto& value = entry.value(); value.is_object() && value.contains("endpoint")) {
            for (const auto& endpoint : value["endpoint"]) {
                for (const auto& engine : endpoint["engine"]) {
                    if (engine == "etcd_rest") {
                        std::string name = entry.key();
                        std::string base = endpoint["base"];
                        std::string stem = endpoint["stem"];
                        schema.add_listener(Listener(name, base, stem));
                    }
                }
            }
        }
    }

    return schema;
}

} // namespace aviso
