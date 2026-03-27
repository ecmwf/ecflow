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

void export_Collections(py::module& m) {
    // Export the vector of Variable
    py::bind_vector<std::vector<Variable>>(m, "VariableList");

    // Export the vector of node_ptr
    py::bind_vector<std::vector<node_ptr>>(
        m, "NodeVec", "Hold a list of Nodes (i.e `suite`_, `family`_ or `task`_\\ s)");

    // Export the vector of suite_ptr
    py::bind_vector<std::vector<suite_ptr>>(m, "SuiteVec");

    // Export the vector of family_ptr
    py::bind_vector<std::vector<family_ptr>>(m, "FamilyVec");

    // Export the vector of task_ptr
    py::bind_vector<std::vector<task_ptr>>(m, "TaskVec");
}
