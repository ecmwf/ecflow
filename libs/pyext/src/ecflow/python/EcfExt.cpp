/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/python.hpp>
#include <boost/python/docstring_options.hpp>

namespace bp = boost::python;

void export_Core();
void export_NodeAttr();
void export_Node();
void export_Task();
void export_SuiteAndFamily();
void export_Defs();
void export_Client();

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects
BOOST_PYTHON_MODULE(ecflow) {
    boost::python::docstring_options doc_options(
        true, // show the docstrings from here
        true, // show Python signatures.
        false // Don't mention the C++ method signatures in the generated docstrings
    );
    bp::scope().attr("__doc__") =
        "The ecflow module provides the python bindings/api for creating definition structure "
        "and communicating with the server.";

    export_Core();
    export_NodeAttr();
    export_Node();
    export_Task();
    export_SuiteAndFamily();
    export_Defs();
    export_Client();
}
