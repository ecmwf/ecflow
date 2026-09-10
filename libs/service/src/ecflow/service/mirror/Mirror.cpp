/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/service/mirror/Mirror.hpp"

namespace ecf::service::mirror {

/* MirrorRequest */

std::ostream& operator<<(std::ostream& os, const MirrorRequest& r) {
    os << "MirrorRequest{";
    os << "attribute=" << r.attribute << ", ";
    os << "path=" << r.path << ", ";
    os << "host=" << r.host << ", ";
    os << "port=" << r.port << ", ";
    os << "polling=" << r.polling << ", ";
    os << "ssl=" << r.ssl << ", ";
    os << "auth=" << r.auth << "}";
    return os;
}

/* MirrorNotification */

std::ostream& operator<<(std::ostream& os, const MirrorNotification& n) {
    os << "MirrorNotification{" << n.path() << ", " << n.data().state << "}";
    return os;
}

/* MirrorError */

std::ostream& operator<<(std::ostream& os, const MirrorError& n) {
    os << "MirrorError{" << n.path() << n.reason() << "}";
    return os;
}

/* MirrorResponse */

std::ostream& operator<<(std::ostream& os, const MirrorResponse& r) {
    std::visit([&os](const auto& v) { os << v; }, r);
    return os;
}

} // namespace ecf::service::mirror
