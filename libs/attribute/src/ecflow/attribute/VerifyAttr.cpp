/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/VerifyAttr.hpp"

#include <sstream>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace ecf;

bool VerifyAttr::operator==(const VerifyAttr& rhs) const {
    if (state_ != rhs.state_) {
        return false;
    }
    if (expected_ != rhs.expected_) {
        return false;
    }
    return true;
}

void VerifyAttr::incrementActual() {
    actual_++;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "VerifyAttr::incrementActual()\n";
#endif
}

void VerifyAttr::reset() {
    actual_          = 0;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "VerifyAttr::reset()\n";
#endif
}

std::string VerifyAttr::toString() const {
    std::stringstream ss;
    ss << "verify " << NState::toString(state_) << Str::COLON() << expected_;
    return ss.str();
}

std::string VerifyAttr::dump() const {
    std::stringstream ss;
    ss << "verify " << NState::toString(state_) << Str::COLON() << expected_;
    ss << " actual(" << actual_ << ")";
    return ss.str();
}

template <class Archive>
void VerifyAttr::serialize(Archive& ar) {
    ar(CEREAL_NVP(state_), CEREAL_NVP(expected_), CEREAL_NVP(actual_));
}
CEREAL_TEMPLATE_SPECIALIZE(VerifyAttr);
