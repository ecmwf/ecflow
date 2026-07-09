/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Collections_HPP
#define ecflow_core_Collections_HPP

namespace ecf {

namespace collections {

namespace detail {

// Range adapter exposing begin()/end() bound to a container's reverse iterators, for use in range-for loops.
template <typename T>
class reverse_adapter {
private:
    // Keeps a reference to the original container
    T& container;

public:
    explicit reverse_adapter(T& c)
        : container(c) {}

    // Map begin() to the container's reverse begin
    auto begin() const { return container.rbegin(); }
    auto begin() { return container.rbegin(); }

    // Map end() to the container's reverse end
    auto end() const { return container.rend(); }
    auto end() { return container.rend(); }
};

} // namespace detail

///
/// @brief Returns an adapter that iterates @p container in reverse order.
///
/// Intended for use in range-for loops, e.g. `for (auto& e : reversed(v))`.
///
/// @tparam T              Container type; must provide rbegin()/rend().
/// @param[in,out] container  Container to iterate in reverse; must outlive the returned adapter.
/// @return An adapter exposing begin()/end() bound to @p container's reverse iterators.
///
template <typename T>
auto reversed(T& container) {
    return detail::reverse_adapter<T>(container);
}

} // namespace collections

} // namespace ecf

#endif /* ecflow_core_Collections_HPP */
