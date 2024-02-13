/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/DState.hpp"

#include <cassert>
#include <stdexcept>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Enumerate.hpp"
#include "ecflow/core/Serialization.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<DState::State>
{
    using underlying_t = std::underlying_type_t<DState::State>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(DState::State::UNKNOWN, "unknown"),
        std::make_pair(DState::State::COMPLETE, "complete"),
        std::make_pair(DState::State::QUEUED, "queued"),
        std::make_pair(DState::State::ABORTED, "aborted"),
        std::make_pair(DState::State::SUBMITTED, "submitted"),
        std::make_pair(DState::State::ACTIVE, "active"),
        std::make_pair(DState::State::SUSPENDED, "suspended"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<DState::State>::size == map.back().first + 1);
};

} // namespace ecf::detail

void DState::setState(State s) {
    st_              = s;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "DState::setState\n";
#endif
}

NState::State DState::convert(DState::State display_state) {
    switch (display_state) {
        case DState::UNKNOWN:
            return NState::UNKNOWN;
        case DState::COMPLETE:
            return NState::COMPLETE;
        case DState::SUSPENDED:
            return NState::UNKNOWN;
        case DState::QUEUED:
            return NState::QUEUED;
        case DState::ABORTED:
            return NState::ABORTED;
        case DState::SUBMITTED:
            return NState::SUBMITTED;
        case DState::ACTIVE:
            return NState::ACTIVE;
    }
    return NState::UNKNOWN;
}

const char* DState::toString(DState::State s) {
    if (auto found = ecf::Enumerate<DState::State>::to_string(s); found) {
        return found.value().data();
    }
    assert(false);
    return {};
}

std::string DState::to_html(DState::State s) {
    std::string res;
    if (auto found = ecf::Enumerate<DState::State>::to_string(s); found) {
        res += "<state>";
        res += found.value();
        res += "</state>";
        return res;
    }
    assert(false);
    return res;
}

DState::State DState::toState(const std::string& str) {
    if (auto found = ecf::Enumerate<DState::State>::to_enum(str); found) {
        return found.value();
    }
    throw std::runtime_error("DState::toState: Can change string to a DState :" + str);
}

bool DState::isValid(const std::string& state) {
    return ecf::Enumerate<DState::State>::is_valid(state);
}

std::vector<std::string> DState::allStates() {
    return ecf::Enumerate<DState::State>::designations();
}

std::vector<DState::State> DState::states() {
    return ecf::Enumerate<DState::State>::enums();
}

// ==========================================================================

template <class Archive>
void DState::serialize(Archive& ar) {
    ar(CEREAL_NVP(st_));
}
CEREAL_TEMPLATE_SPECIALIZE(DState);
