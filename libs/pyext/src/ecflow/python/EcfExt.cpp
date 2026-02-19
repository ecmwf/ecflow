/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Version.hpp"
#include "ecflow/python/PythonBinding.hpp"

void export_Collections(py::module& m);
void export_Core(py::module& m);
void export_NodeAttr(py::module& m);
void export_Node(py::module& m);
void export_Task(py::module& m);
void export_SuiteAndFamily(py::module& m);
void export_Defs(py::module& m);
void export_Client(py::module& m);

PYBIND11_MODULE(ecflow, m) {
    py::options options;
    options.enable_user_defined_docstrings(); // show the docstrings from here
    options.enable_function_signatures();     // show Python signatures.
    options.enable_enum_members_docstring();

    m.doc() = "The ecflow module provides the python bindings/api for creating definition structure "
              "and communicating with the server.";

    m.attr("__version__") = ecf::Version::base();

    export_Collections(m);
    export_Core(m);
    export_NodeAttr(m);
    export_Node(m);
    export_Task(m);
    export_SuiteAndFamily(m);
    export_Defs(m);
    export_Client(m);
}
