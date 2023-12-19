/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/AvisoAttr.hpp"

#include <sstream>

#include "ecflow/attribute/AvisoService.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"

namespace ecf {

AvisoAttr::AvisoAttr(name_t name, listener_t listener) : name_{std::move(name)}, listener_{std::move(listener)} {
    if (!ecf::Str::valid_name(name_)) {
        throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
    }
};

bool AvisoAttr::why(std::string& theReasonWhy) const {
    if (isFree()) {
        return false;
    }

    theReasonWhy += ecf::Message(" is Aviso dependent (", listener_, "), but no notification received");
    return true;
};

bool AvisoAttr::isFree() const {
    using namespace ecf;
    auto& registry = AvisoService::service();
    LOG(Log::DBG, Message("******************** Check Aviso attribute (name: ", name_, ", listener: ", listener_, ")"));
    return registry.check_notification(name_);
}

void AvisoAttr::requeue() {
}

} // namespace ecf
