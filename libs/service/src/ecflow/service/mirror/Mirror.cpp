/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/service/mirror/Mirror.hpp"

namespace ecf::service::mirror {

/* MirrorRequest */

std::ostream& operator<<(std::ostream& os, const MirrorRequest& r) {
    os << "MirrorRequest{";
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
    os << "MirrorNotification{" << n.path() << ", " << n.status() << "}";
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
