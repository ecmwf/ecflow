/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_mirror_Mirror_HPP
#define ecflow_service_mirror_Mirror_HPP

#include <string>
#include <string_view>

#include "ecflow/service/Controller.hpp"
#include "ecflow/service/executor/PeriodicTaskExecutor.hpp"
#include "ecflow/service/mirror/MirrorClient.hpp"

namespace ecf::service::mirror {

/**
 *
 * A MirrorRequest is a request to start listening for state notifications from remote ecFlow server.
 *
 */
class MirrorRequest {
public:
    std::string path;
    std::string host;
    std::string port;
    std::uint32_t polling;
    bool ssl;
    std::string auth;

    friend std::ostream& operator<<(std::ostream&, const MirrorRequest&);
};

/**
 *
 * A MirrorNotification is a notification of a state change in a remote ecFlow server.
 *
 */
struct MirrorNotification
{
    bool success;
    std::string path;
    std::string failure_reason;
    int status;

    std::string reason() const { return failure_reason.substr(0, failure_reason.find_first_of("\n")); }

    friend std::ostream& operator<<(std::ostream&, const MirrorNotification&);
};

/**
 *
 * A MirrorError is a notification that an error occurred when contacting a remote ecFlow server.
 *
 */
class MirrorError {
public:
    explicit MirrorError(std::string_view reason) : reason_{reason.substr(0, reason.find_first_of("\n"))} {}

    const std::string& reason() const { return reason_; }

    friend std::ostream& operator<<(std::ostream&, const MirrorError&);

private:
    std::string reason_;
};

/**
 *
 * A MirrorResponse is a notification of either state change or an error.
 *
 */
using MirrorResponse = std::variant<MirrorNotification, MirrorError>;

std::ostream& operator<<(std::ostream&, const MirrorResponse&);

} // namespace ecf::service::mirror

#endif /* ecflow_service_mirror_Mirror_HPP */
