/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_aviso_ListenerSchema_HPP
#define ecflow_service_aviso_ListenerSchema_HPP

#include <optional>
#include <string>
#include <unordered_map>

#include "ecflow/service/aviso/Listener.hpp"

namespace ecf::service::aviso {

class ListenerSchema {
public:
    inline void add_listener(const Listener& listener) {
        auto name        = std::string(listener.name());
        listeners_[name] = listener;
    };
    inline std::optional<Listener> get_listener(const std::string& name) const {
        if (auto found = listeners_.find(name); found != listeners_.end()) {
            return found->second;
        }
        return std::nullopt;
    }

    static ListenerSchema load(const std::string& schema_path);
    static ListenerSchema load(std::istream& schema_stream);

private:
    std::unordered_map<std::string, Listener> listeners_{};
};

} // namespace ecf::service::aviso

#endif /* ecflow_service_aviso_ListenerSchema_HPP */
