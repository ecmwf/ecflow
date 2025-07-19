/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/Edit.hpp"

#include <stdexcept>

#include "ecflow/python/PythonUtil.hpp"

Edit::Edit(const py::dict& dict) {
    pyutil_dict_to_str_vec(dict, vec_);
}

Edit::Edit(const py::kwargs& kwargs) {
    pyutil_dict_to_str_vec(kwargs, vec_);
}

Edit::Edit(const py::dict& dict, const py::kwargs& kwargs) {
    pyutil_dict_to_str_vec(dict, vec_);
    pyutil_dict_to_str_vec(kwargs, vec_);
}
