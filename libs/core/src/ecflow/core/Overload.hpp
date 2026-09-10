/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_Overload_HPP
#define ecflow_core_Overload_HPP

namespace ecf {

template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>; // Deduction guideline not needed from C++20

} // namespace ecf

#endif /* ecflow_core_Overload_HPP */
