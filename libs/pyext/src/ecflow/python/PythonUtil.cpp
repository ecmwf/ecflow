/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/PythonUtil.hpp"

#include <stdexcept>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Converter.hpp"

void pyutil_list_to_int_vec(const py::list& list, std::vector<int>& int_vec) {
    auto the_list_size = len(list);
    int_vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        int_vec.push_back(py::extract<int>(list[i]));
    }
}

void pyutil_list_to_str_vec(const py::list& list, std::vector<std::string>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        vec.push_back(py::extract<std::string>(list[i]));
    }
}

void pyutil_list_to_str_vec(const py::list& list, std::vector<Variable>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        vec.push_back(py::extract<Variable>(list[i]));
    }
}

void pyutil_dict_to_str_vec(const py::dict& dict,
                                      std::vector<std::pair<std::string, std::string>>& str_pair_vec) {
    py::list keys = dict.keys();
    const auto no_of_keys    = len(keys);
    str_pair_vec.reserve(no_of_keys);

    for (ssize_t i = 0; i < no_of_keys; ++i) {

        std::string second;
        std::string first = py::extract<std::string>(keys[i]);
        if (py::extract<std::string>(dict[keys[i]]).check()) {
            second = py::extract<std::string>(dict[keys[i]]);
        }
        else if (py::extract<int>(dict[keys[i]]).check()) {
            int the_int = py::extract<int>(dict[keys[i]]);
            second      = ecf::convert_to<std::string>(the_int);
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: type not convertible to string or integer");
        }
        str_pair_vec.emplace_back(first, second);
    }
}

void pyutil_dict_to_str_vec(const py::dict& dict, std::vector<Variable>& vec) {
    py::list keys = dict.keys();
    const auto no_of_keys    = len(keys);
    vec.reserve(no_of_keys);

    for (ssize_t i = 0; i < no_of_keys; ++i) {

        std::string second;
        std::string first = py::extract<std::string>(keys[i]);
        if (py::extract<std::string>(dict[keys[i]]).check()) {
            second = py::extract<std::string>(dict[keys[i]]);
        }
        else if (py::extract<int>(dict[keys[i]]).check()) {
            int the_int = py::extract<int>(dict[keys[i]]);
            second      = ecf::convert_to<std::string>(the_int);
        }
        else
            throw std::runtime_error("PythonUtil::dict_to_str_vec: type not convertible to string or integer");

        vec.emplace_back(first, second);
    }
}
