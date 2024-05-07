/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/MirrorAttr.hpp"

#include <sstream>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

MirrorAttr::MirrorAttr(Node* parent,
                       name_t name,
                       remote_path_t remote_path,
                       remote_host_t remote_host,
                       remote_port_t remote_port,
                       polling_t polling)
    : parent_{parent},
      name_{std::move(name)},
      remote_path_{std::move(remote_path)},
      remote_host_{std::move(remote_host)},
      remote_port_{std::move(remote_port)},
      polling_{std::move(polling)} {
    //      controller_{nullptr},
    //      runner_{nullptr} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid MirrorAttr name :", name_));
    }
};

bool MirrorAttr::why(std::string& theReasonWhy) const {
    if (isFree()) {
        return false;
    }

    theReasonWhy += ecf::Message(" is a Mirror of ", remote_path(), " at '", remote_host(), ":", remote_port(), "'");
    return true;
};

void MirrorAttr::reset() {
    state_change_no_ = Ecf::incr_state_change_no();

    ALOG(D,
         "MirrorAttr::reset: start polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                         << ", port: " << remote_port_ << ")");

    start_controller();
}

bool MirrorAttr::isFree() const {

    LOG(Log::MSG, "**** Check Mirror attribute (name: " << name_ << ")");

    start_controller();

    return true;
}

void MirrorAttr::start() const {
    // Nothing do do...
}

void MirrorAttr::finish() const {
    // Nothing do do...
}

void MirrorAttr::start_controller() const {
    if (controller_ == nullptr) {
        ALOG(D,
             "MirrorAttr::reset: start polling for Mirror attribute (name: " << name_ << ", host: " << remote_host_
                                                                             << ", port: " << remote_port_ << ")");

        // Controller -- start up the Mirror controller
        controller_ = std::make_shared<ecf::service::MirrorController>();
        controller_->subscribe(ecf::service::MirrorRequest{
            remote_path_, remote_host_, remote_port_, boost::lexical_cast<std::uint32_t>(polling_)});
        // Runner -- start up the Mirror runner, and thus effectively start the Aviso listener
        // n.b. this must be done after subscribing in the controller, so that the polling interval is set
        runner_ = std::make_shared<ecf::service::MirrorRunner>(*controller_);
        runner_->start();
    }
}

} // namespace ecf
