/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/NOrder.hpp"

#include <cassert>

#include "ecflow/core/Enumerate.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<NOrder::Order>
{
    using underlying_t = std::underlying_type_t<NOrder::Order>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(NOrder::Order::TOP, "top"),
        std::make_pair(NOrder::Order::BOTTOM, "bottom"),
        std::make_pair(NOrder::Order::ALPHA, "alpha"),
        std::make_pair(NOrder::Order::ORDER, "order"),
        std::make_pair(NOrder::Order::UP, "up"),
        std::make_pair(NOrder::Order::DOWN, "down"),
        std::make_pair(NOrder::Order::RUNTIME, "runtime")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<NOrder::Order>::size == map.back().first + 1);
};

} // namespace ecf::detail

std::string NOrder::toString(NOrder::Order s) {
    if (auto found = ecf::Enumerate<NOrder::Order>::to_string(s); found) {
        return std::string{found.value()};
    }
    assert(false);
    return std::string{};
}

NOrder::Order NOrder::toOrder(const std::string& str) {
    if (auto found = ecf::Enumerate<NOrder::Order>::to_enum(str); found) {
        return found.value();
    }
    assert(false);
    return NOrder::TOP;
}

bool NOrder::isValid(const std::string& order) {
    return ecf::Enumerate<NOrder::Order>::is_valid(order);
}
