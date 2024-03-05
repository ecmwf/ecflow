/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/AvisoAttr.hpp"

#include <sstream>

#include "ecflow/core/Message.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/service/Registry.hpp"

namespace ecf {

AvisoAttr::AvisoAttr(Node* parent, name_t name, listener_t listener)
    : parent_{parent},
      name_{std::move(name)},
      listener_{std::move(listener)} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
    }
};

std::string AvisoAttr::path() const {
    std::string path = parent_->absNodePath();
    path += ':';
    path += name_;
    return path;
}

bool AvisoAttr::why(std::string& theReasonWhy) const {
    if (isFree()) {
        return false;
    }

    theReasonWhy += ecf::Message(" is Aviso dependent (", listener_, "), but no notification received");
    return true;
};

void AvisoAttr::reset() {
    // TODO: Implement...
}

bool AvisoAttr::isFree() const {
    using namespace ecf;
    auto& controller =
        ecf::service::GlobalRegistry::instance().get_service<ecf::service::AvisoController>("controller");

    std::string aviso_path = path();

    LOG(Log::DBG, Message("**** Check Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    // Task associated with Attribute is free when any notification is found
    auto notifications = controller.poll_notifications(aviso_path);
    return !notifications.empty();
}

void AvisoAttr::start() const {
    using namespace ecf;
    auto& controller =
        ecf::service::GlobalRegistry::instance().get_service<ecf::service::AvisoController>("controller");

    LOG(Log::DBG, Message("**** Subscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    std::string aviso_path = path();
    std::string aviso_url;
    auto found_aviso_url = parent_->findParentVariableValue("ECF_AVISO_URL", aviso_url);

    if (!found_aviso_url) {
        throw std::runtime_error("AvisoAttr::requeue: Could not find ECF_AVISO_URI for " + aviso_path);
    }

    auto cfg = listener_;
    cfg      = cfg.substr(1, cfg.size() - 2);
    return controller.subscribe(aviso::ListenRequest::make_listen_start(aviso_path, aviso_url, cfg));
}

void AvisoAttr::finish() const {
    using namespace ecf;
    auto& controller =
        ecf::service::GlobalRegistry::instance().get_service<ecf::service::AvisoController>("controller");

    LOG(Log::DBG, Message("**** Unsubscribe Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));

    std::string aviso_path = path();
    return controller.unsubscribe(aviso::ListenRequest::make_listen_finish(aviso_path));
}

} // namespace ecf
