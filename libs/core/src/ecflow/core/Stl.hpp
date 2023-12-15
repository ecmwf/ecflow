/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Stl_HPP
#define ecflow_core_Stl_HPP

#include <algorithm>
#include <memory>

namespace ecf {
/// Helper struct that will aid the deletion of Pointer from a container
template <typename T>
struct TSeqDeletor
{
    void operator()(T pointer) const {
        // std::cout << "Destroy of this pointer" << std::endl;
        delete pointer;
        pointer = 0;
    }
};
/// This function can be used to delete the pointers in a container
/// i.e int main (int argc, char **argv) {
///        std::vector <std::string *> vect;
///        vect.push_back (new std::string ("Stephane"));
///        DeletePtrs (vect);
///     }
template <typename Container>
void DeletePtrs(Container& pContainer) {
    std::for_each(pContainer.begin(), pContainer.end(), TSeqDeletor<typename Container::value_type>());
    pContainer.clear();
}

/// Helper struct that will aid the deletion of Pointer from a Associative container
template <typename TPair>
struct TAsoDeletor
{
    void operator()(TPair& tElem) const {
        if (tElem.second) {
            delete tElem.second;
        }
    }
};
/// This function can be used to delete the pointers in a Assoc container
/// i.e int main (int argc, char **argv) {
///        std::map <int,std::string *> theMap;
///        theMap[0] =  new std::string ("Stephane");
///        AssoDeletePtrs(theMap);
///     }
template <typename Container>
void AssoDeletePtrs(Container& pContainer) {
    std::for_each(pContainer.begin(), pContainer.end(), TAsoDeletor<typename Container::value_type>());
    pContainer.clear();
}

namespace algorithm {

namespace detail {

template <typename T>
struct is_shared_pointer : std::false_type
{
};

template <typename T>
struct is_shared_pointer<std::shared_ptr<T>> : std::true_type
{
};

} // namespace detail

template <typename T>
constexpr bool is_shared_pointer_v = detail::is_shared_pointer<T>::value;

template <typename C, typename Predicate>
inline auto find_by(C& container, Predicate predicate) {
    return std::find_if(std::begin(container), std::end(container), predicate);
}

template <typename C>
inline auto find_by_name(C& container, std::string_view name) {
    // Important: special handling to seamlessly handle containers of std::shared_ptr.
    if constexpr (is_shared_pointer_v<typename C::value_type>) {
        return find_by(container, [&](const auto& item) { return item->name() == name; });
    }
    else {
        return find_by(container, [&](const auto& item) { return item.name() == name; });
    }
}

template <typename C, typename I>
inline auto find_by_number(C& container, I number) {
    return find_by(container, [&](const auto& item) { return item.number() == number; });
}

} // namespace algorithm

} // namespace ecf

#endif /* ecflow_core_Stl_HPP */
