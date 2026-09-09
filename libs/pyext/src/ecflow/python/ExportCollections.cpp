/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/python/ExportCollections.hpp"

#include <pybind11/stl_bind.h>

#include "ecflow/python/PythonUtil.hpp"

void export_Collections(py::module& m) {
    // Export the vector of Variable
    {
        py::object vl_cls =
            py::bind_vector<std::vector<Variable>>(m, "VariableList", py::dynamic_attr(), "Hold a list of Variables");
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
        m, "NodeVec", "Hold a list of Nodes (i.e :term:`suite`, :term:`family` or :term:`task`\\ s)");

    // Export the vector of suite_ptr
    py::bind_vector<std::vector<suite_ptr>>(m, "SuiteVec", "Hold a list of :term:`suite` nodes's");

    // Export the vector of family_ptr
    py::bind_vector<std::vector<family_ptr>>(m, "FamilyVec", "Hold a list of :term:`family` nodes");

    // Export the vector of task_ptr
    py::bind_vector<std::vector<task_ptr>>(m, "TaskVec", "Hold a list of :term:`task` nodes");
}
