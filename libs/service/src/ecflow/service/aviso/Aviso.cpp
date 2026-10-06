// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/service/aviso/Aviso.hpp"

#include <ostream>

#include "ecflow/core/Overload.hpp"

namespace ecf::service::aviso {

std::ostream& operator<<(std::ostream& os, const AvisoSubscribe& request) {
    os << "AvisoSubscribe{";
    os << "path: " << request.path();
    os << ", listener: " << request.listener();
    os << ", url: " << request.url();
    os << ", revision: " << request.revision();
    os << ", auth: " << request.auth();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification) {
    os << "AvisoNotification{";
    os << "event_type: " << notification.event_type();
    os << ", sequence: " << notification.sequence();
    os << ", identifier: " << notification.identifier_json();
    os << ", payload: " << notification.payload_json();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoError& error) {
    os << "AvisoError{";
    os << "reason: " << error.reason();
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const AvisoResponse& response) {
    std::visit(ecf::overload{[&os](const AvisoNotification& r) { os << r; }, [&os](const AvisoError& r) { os << r; }},
               response);
    return os;
}

} // namespace ecf::service::aviso
