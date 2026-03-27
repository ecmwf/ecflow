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
    for (size_t i = 0; i < the_list_size; ++i) {
        int_vec.push_back(list[i].cast<int>());
    }
}

void pyutil_list_to_str_vec(const py::list& list, std::vector<std::string>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (size_t i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<std::string>());
    }
}

void pyutil_list_to_str_vec(const py::list& list, std::vector<Variable>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (size_t i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<Variable>());
    }
}

void pyutil_dict_to_str_vec(const py::dict& dict, std::vector<std::pair<std::string, std::string>>& vec) {

    for (auto entry : dict) {
        std::string first;
        if (auto found = py_extract<py::str>(entry.first); found) {
            first = found.value();
        }
        else if (auto found = py_extract<std::string>(entry.first); found) {
            first = found.value();
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: key not convertible to py::str or std::string");
        }

        std::string second;
        if (auto found = py_extract<std::string>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<py::str>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<int>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else if (auto found = py_extract<py::int_>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: type not convertible to string or integer");
        }

        vec.emplace_back(first, second);
    }
}

void pyutil_dict_to_str_vec(const py::dict& dict, std::vector<Variable>& vec) {

    for (auto entry : dict) {
        std::string first;
        if (auto found = py_extract<py::str>(entry.first); found) {
            first = found.value();
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: key not convertible to string");
        }

        std::string second;
        if (auto found = py_extract<py::str>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<py::int_>(entry.second); found) {
            int value = found.value();
            second    = ecf::convert_to<std::string>(value);
        }
        else {
            throw std::runtime_error("PythonUtil::dict_to_str_vec: value not convertible to string or integer");
        }

        vec.emplace_back(first, second);
    }
}
