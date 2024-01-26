/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/NState.hpp"

#include <cassert>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Enumerate.hpp"
#include "ecflow/core/Serialization.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<NState::State>
{
    using underlying_t = std::underlying_type_t<NState::State>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(NState::State::UNKNOWN, "unknown"),
        std::make_pair(NState::State::COMPLETE, "complete"),
        std::make_pair(NState::State::QUEUED, "queued"),
        std::make_pair(NState::State::ABORTED, "aborted"),
        std::make_pair(NState::State::SUBMITTED, "submitted"),
        std::make_pair(NState::State::ACTIVE, "active")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<NState::State>::size == map.back().first + 1);
};

} // namespace ecf::detail

void NState::setState(State s) {
    st_              = s;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "NState::setState\n";
#endif
}

const char* NState::toString(NState::State s) {
    if (auto found = ecf::Enumerate<NState::State>::to_string(s); found) {
        return found.value().data();
    }
    assert(false);
    return nullptr;
}

std::string NState::to_html(NState::State s) {
    std::string res;
    if (auto found = ecf::Enumerate<NState::State>::to_string(s); found) {
        res += "<state>";
        res += found.value();
        res += "</state>";
        return res;
    }
    assert(false);
    return res;
}

NState::State NState::toState(const std::string& str) {
    if (auto found = ecf::Enumerate<NState::State>::to_enum(str); found) {
        return found.value();
    }
    assert(false);
    return NState::UNKNOWN;
}

std::pair<NState::State, bool> NState::to_state(const std::string& str) {
    if (auto found = ecf::Enumerate<NState::State>::to_enum(str); found) {
        return std::make_pair(found.value(), true);
    }
    return std::make_pair(NState::UNKNOWN, false);
}

bool NState::isValid(const std::string& state) {
    return ecf::Enumerate<NState::State>::is_valid(state);
}

std::vector<std::string> NState::allStates() {
    return ecf::Enumerate<NState::State>::designations();
}

std::vector<NState::State> NState::states() {
    return ecf::Enumerate<NState::State>::enums();
}

template <class Archive>
void NState::serialize(Archive& ar) {
    ar(CEREAL_NVP(st_));
}
CEREAL_TEMPLATE_SPECIALIZE(NState);
