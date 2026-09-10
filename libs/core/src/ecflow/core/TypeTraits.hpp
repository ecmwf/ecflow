/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_TypeTraits_HPP
#define ecflow_core_TypeTraits_HPP

#include <type_traits>

namespace ecf {

///
/// @brief Check if a given reference type is one of the comparing types
///
/// The value data member is true when the reference type is one of the comparing types, otherwise false.
///
/// @tparam T the reference type
/// @tparam Xs the comparing types
///
template <typename T, typename... Xs>
struct is_one_of
{
    static constexpr bool value = false;
};

///
/// @brief Specialisation of is_one_of to handle the comparison between the reference type and the comparing types
///
/// @tparam T the reference type
/// @tparam X the first of the comparing types
/// @tparam Xs the remaining comparing types
///
template <typename T, typename X, typename... Xs>
struct is_one_of<T, X, Xs...>
{
    static constexpr bool value = std::is_same_v<T, X> || is_one_of<T, Xs...>::value;
};

///
/// @brief A convenience variable template to get the value of is_one_of without needing to access the value data member
///
/// @tparam T the reference type
/// @tparam Xs the comparing types
///
template <typename T, typename... Xs>
inline constexpr bool is_one_of_v = is_one_of<T, Xs...>::value;

} // namespace ecf

#endif /* ecflow_core_TypeTraits_HPP */
