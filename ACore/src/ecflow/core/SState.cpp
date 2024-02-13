/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/SState.hpp"

#include <cassert>

#include "ecflow/core/Enumerate.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<SState::State>
{
    using underlying_t = std::underlying_type_t<SState::State>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(SState::State::HALTED, "HALTED"),
        std::make_pair(SState::State::SHUTDOWN, "SHUTDOWN"),
        std::make_pair(SState::State::RUNNING, "RUNNING")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<SState::State>::size == map.back().first + 1);
};

} // namespace ecf::detail

std::string SState::to_string(int status) {
    return to_string(static_cast<SState::State>(status));
}

std::string SState::to_string(SState::State state) {
    if (auto found = ecf::Enumerate<SState::State>::to_string(state); found) {
        return std::string{found.value()};
    }
    return "UNKNOWN??";
}

SState::State SState::toState(const std::string& str) {
    if (auto found = ecf::Enumerate<SState::State>::to_enum(str); found) {
        return found.value();
    }
    assert(false);
    return SState::HALTED;
}

bool SState::isValid(const std::string& state) {
    return ecf::Enumerate<SState::State>::is_valid(state);
}
