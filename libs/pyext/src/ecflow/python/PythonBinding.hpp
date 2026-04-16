/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_PythonBinding_HPP
#define ecflow_python_PythonBinding_HPP

#include <pybind11/iostream.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "ecflow/node/NodeFwd.hpp"

namespace py = pybind11;

template <typename T>
std::optional<T> py_extract(py::object obj) {
    if (obj.is_none()) {
        return std::nullopt;
    }

    if constexpr (std::is_same_v<T, node_ptr>) {
        // special handling of node_ptr, as py::isinstance<node_ptr> doesn't work due to node_ptr being a shared_ptr
        return py::cast<T>(obj);
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
    if (py::isinstance<T>(obj)) {
        return py::cast<T>(obj);
    }
    else {
        return std::nullopt;
    }
}

#endif /* ecflow_python_PythonBinding_HPP */
