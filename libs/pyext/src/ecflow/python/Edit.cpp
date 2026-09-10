/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/python/Edit.hpp"

#include <stdexcept>

#include "ecflow/python/PythonUtil.hpp"

Edit::Edit(const py::dict& dict) {
    py_dict_to_str_vec(dict, vec_);
}

Edit::Edit(const py::kwargs& kwargs) {
    py_dict_to_str_vec(kwargs, vec_);
}

Edit::Edit(const py::dict& dict, const py::kwargs& kwargs) {
    py_dict_to_str_vec(dict, vec_);
    py_dict_to_str_vec(kwargs, vec_);
}

Edit::Edit(const py::dict& dict1, const py::dict& dict2) {
    py_dict_to_str_vec(dict1, vec_);
    py_dict_to_str_vec(dict2, vec_);
}
