/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/python/ExportCollections.hpp"

#include <pybind11/stl_bind.h>

#include "ecflow/python/PythonUtil.hpp"

void export_Collections(py::module& m) {
    // Export the vector of Variable
    {
        py::object vl_cls = py::bind_vector<std::vector<Variable>>(m, "VariableList", "Hold a list of Variables");
        vl_cls.attr("__eq__") =
            py::cpp_function([](const py::object& self, const py::object& other) -> bool { return self.is(other); },
                             py::name("__eq__"),
                             py::is_method(vl_cls),
                             py::arg("other"),
                             py::pos_only());
        vl_cls.attr("__ne__") =
            py::cpp_function([](const py::object& self, const py::object& other) -> bool { return !self.is(other); },
                             py::name("__ne__"),
                             py::is_method(vl_cls),
                             py::arg("other"),
                             py::pos_only());
        vl_cls.attr("__hash__") = py::cpp_function(
            [](const py::object& self) -> auto { return py_hash(self); }, py::name("__hash__"), py::is_method(vl_cls));
    }

    // Export the vector of node_ptr
    py::bind_vector<std::vector<node_ptr>>(
        m, "NodeVec", "Hold a list of Nodes (i.e `suite`_, `family`_ or `task`_\\ s)");

    // Export the vector of suite_ptr
    py::bind_vector<std::vector<suite_ptr>>(m, "SuiteVec", "Hold a list of `suite`_ nodes's");

    // Export the vector of family_ptr
    py::bind_vector<std::vector<family_ptr>>(m, "FamilyVec", "Hold a list of `family`_ nodes");

    // Export the vector of task_ptr
    py::bind_vector<std::vector<task_ptr>>(m, "TaskVec", "Hold a list of `task`_ nodes");
}
