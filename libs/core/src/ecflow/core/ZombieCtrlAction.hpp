/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_ZombieCtrlAction_HPP
#define ecflow_core_ZombieCtrlAction_HPP

#include <cassert>
#include <string>

#include "ecflow/core/Enumerate.hpp"

namespace ecf {

enum class ZombieCtrlAction { FOB, FAIL, ADOPT, REMOVE, BLOCK, KILL };

namespace detail {

template <>
struct EnumTraits<ZombieCtrlAction>
{
    using underlying_t = std::underlying_type_t<ZombieCtrlAction>;

    /**
     * The mapping between enum values and their designations
     *
     * @note The order of the entries in the array must be the same as the order of the enum values
     */
    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(ZombieCtrlAction::FOB, "fob"),
        std::make_pair(ZombieCtrlAction::FAIL, "fail"),
        std::make_pair(ZombieCtrlAction::ADOPT, "adopt"),
        std::make_pair(ZombieCtrlAction::REMOVE, "remove"),
        std::make_pair(ZombieCtrlAction::BLOCK, "block"),
        std::make_pair(ZombieCtrlAction::KILL, "kill"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<ZombieCtrlAction>::size == static_cast<underlying_t>(map.back().first) + 1);
};

} // namespace detail

inline std::string to_string(ZombieCtrlAction uc) {
    if (auto found = Enumerate<ZombieCtrlAction>::to_string(uc); found) {
        return std::string{found.value()};
    }
    assert(false);
    return std::string();
}

} // namespace ecf

#endif /* ecflow_core_ZombieCtrlAction_HPP */
