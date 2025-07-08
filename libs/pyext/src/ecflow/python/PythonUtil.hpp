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

#include "ecflow/python/PythonBinding.hpp"

class Variable;

/// Convert a python list to a vector of integers, raising a type error if integer extraction fails
void pyutil_list_to_int_vec(const py::list& list, std::vector<int>& int_vec);
void pyutil_list_to_str_vec(const py::list& list, std::vector<std::string>& int_vec);
void pyutil_list_to_str_vec(const py::list& list, std::vector<Variable>& vec);
void pyutil_dict_to_str_vec(const py::dict& dict, std::vector<std::pair<std::string, std::string>>& str_pair);
void pyutil_dict_to_str_vec(const py::dict& dict, std::vector<Variable>& vec);

template <typename T>
inline const T pyutil_copy_object(const T& v) {
    return v;
}

#endif /* ecflow_python_PythonUtil_HPP */
