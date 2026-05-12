/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_PythonUtil_HPP
#define ecflow_python_PythonUtil_HPP

#include <vector>

#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/python/PythonBinding.hpp"

template <typename T>
std::optional<T> py_extract(py::object obj) {
    if (obj.is_none()) {
        return std::nullopt;
    }

    if constexpr (std::is_same_v<T, node_ptr>) {
        // Attempting the cast to T allows to also handle Node derived classes (e.g. Suite, Family, Task, Alias).
        try {
            return py::cast<T>(obj);
        }
        catch (const py::cast_error&) {
            return std::nullopt;
        }
    }
    else if (py::isinstance<T>(obj)) {
        return py::cast<T>(obj);
    }
    else {
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> py_extract(py::handle obj) {
    if (obj.is_none()) {
        return std::nullopt;
    }
    if constexpr (std::is_same_v<T, node_ptr>) {
        // Attempting the cast to T allows to also handle Node derived classes (e.g. Suite, Family, Task, Alias).
        try {
            return py::cast<T>(obj);
        }
        catch (const py::cast_error&) {
            return std::nullopt;
        }
    }
    else if (py::isinstance<T>(obj)) {
        return py::cast<T>(obj);
    }
    else {
        return std::nullopt;
    }
}

/// Convert a python list to a vector of integers, raising a type error if integer extraction fails
void py_list_to_int_vec(const py::list& list, std::vector<int>& int_vec);

/// Convert a python list to a vector of strings, raising a type error if integer extraction fails
void py_list_to_str_vec(const py::list& list, std::vector<std::string>& int_vec);

/// Convert a python list to a vector of variables, raising a type error if integer extraction fails
void py_list_to_str_vec(const py::list& list, std::vector<Variable>& vec);

/// Convert a python dict to a vector of string pairs, raising a type error if integer extraction fails
void py_dict_to_str_vec(const py::dict& dict, std::vector<std::pair<std::string, std::string>>& str_pair);

/// Convert a python dict to a vector of variables, raising a type error if integer extraction fails
void py_dict_to_str_vec(const py::dict& dict, std::vector<Variable>& vec);

template <typename T>
inline const T pyutil_copy_object(const T& v) {
    return v;
}

py::object py_id(const py::object& self);

ssize_t py_hash(const py::object& self);

#endif /* ecflow_python_PythonUtil_HPP */
