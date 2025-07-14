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
Edit::Edit(const py::dict& dict, const py::dict& dict2) {
    pyutil_dict_to_str_vec(dict, vec_);
    pyutil_dict_to_str_vec(dict2, vec_);
}

py::object Edit::init(py::tuple args, py::dict kw) {
    // cout << "Edit::init args: " << len(args) << " kwargs " << len(kw) << "\n";
    //  args[0] is Edit(i.e self)
    for (int i = 1; i < len(args); ++i) {
        if (py::extract<py::dict>(args[i]).check()) {
            py::dict d = py::extract<py::dict>(args[i]);
            return args[0].attr("__init__")(d, kw); // calls -> .def(init<dict,dict>() -> Edit(dict,dict)
        }
        else
            throw std::runtime_error("Edit::Edit: only accepts dictionary and key word arguments");
    }
    py::tuple rest(args.slice(1, py::_));
    return args[0].attr("__init__")(kw); // calls -> .def(init<dict>() -> Edit(const py::dict& dict)
}
