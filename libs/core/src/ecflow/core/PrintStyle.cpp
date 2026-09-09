/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/core/PrintStyle.hpp"

#include "ecflow/core/Enumerate.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<PrintStyle::Type_t>
{
    using underlying_t = std::underlying_type_t<PrintStyle::Type_t>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(PrintStyle::Type_t::NOTHING, "NOTHING"),
        std::make_pair(PrintStyle::Type_t::DEFS, "DEFS"),
        std::make_pair(PrintStyle::Type_t::STATE, "STATE"),
        std::make_pair(PrintStyle::Type_t::MIGRATE, "MIGRATE"),
        std::make_pair(PrintStyle::Type_t::NET, "NET"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<PrintStyle::Type_t>::size == map.back().first + 1);
};

} // namespace ecf::detail

bool PrintStyle::is_persist_style(PrintStyle::Type_t t) {
    return t == PrintStyle::MIGRATE || t == PrintStyle::NET;
}

std::string PrintStyle::to_string(PrintStyle::Type_t t) {
    return std::string{ecf::Enumerate<PrintStyle::Type_t>::to_string(t).value_or("")};
}
