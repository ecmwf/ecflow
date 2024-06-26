/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/BoostPythonUtil.hpp"

#include <stdexcept>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Converter.hpp"

void BoostPythonUtil::list_to_int_vec(const boost::python::list& list, std::vector<int>& int_vec) {
    auto the_list_size = len(list);
    int_vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        int_vec.push_back(boost::python::extract<int>(list[i]));
    }
}

void BoostPythonUtil::list_to_str_vec(const boost::python::list& list, std::vector<std::string>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        vec.push_back(boost::python::extract<std::string>(list[i]));
    }
}

void BoostPythonUtil::list_to_str_vec(const boost::python::list& list, std::vector<Variable>& vec) {
    auto the_list_size = len(list);
    vec.reserve(the_list_size);
    for (ssize_t i = 0; i < the_list_size; ++i) {
        vec.push_back(boost::python::extract<Variable>(list[i]));
    }
}

void BoostPythonUtil::dict_to_str_vec(const boost::python::dict& dict,
                                      std::vector<std::pair<std::string, std::string>>& str_pair_vec) {
    boost::python::list keys = dict.keys();
    const auto no_of_keys    = len(keys);
    str_pair_vec.reserve(no_of_keys);

    for (ssize_t i = 0; i < no_of_keys; ++i) {

        std::string second;
        std::string first = boost::python::extract<std::string>(keys[i]);
        if (boost::python::extract<std::string>(dict[keys[i]]).check()) {
            second = boost::python::extract<std::string>(dict[keys[i]]);
        }
        else if (boost::python::extract<int>(dict[keys[i]]).check()) {
            int the_int = boost::python::extract<int>(dict[keys[i]]);
            second      = ecf::convert_to<std::string>(the_int);
        }
        else
            throw std::runtime_error("BoostPythonUtil::dict_to_str_vec: type not convertible to string or integer");
        str_pair_vec.emplace_back(first, second);
    }
}

void BoostPythonUtil::dict_to_str_vec(const boost::python::dict& dict, std::vector<Variable>& vec) {
    boost::python::list keys = dict.keys();
    const auto no_of_keys    = len(keys);
    vec.reserve(no_of_keys);

    for (ssize_t i = 0; i < no_of_keys; ++i) {

        std::string second;
        std::string first = boost::python::extract<std::string>(keys[i]);
        if (boost::python::extract<std::string>(dict[keys[i]]).check()) {
            second = boost::python::extract<std::string>(dict[keys[i]]);
        }
        else if (boost::python::extract<int>(dict[keys[i]]).check()) {
            int the_int = boost::python::extract<int>(dict[keys[i]]);
            second      = ecf::convert_to<std::string>(the_int);
        }
        else
            throw std::runtime_error("BoostPythonUtil::dict_to_str_vec: type not convertible to string or integer");

        vec.emplace_back(first, second);
    }
}
