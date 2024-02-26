/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_NodeUtil_HPP
#define ecflow_python_NodeUtil_HPP

#include <boost/python.hpp>

#include "ecflow/node/NodeFwd.hpp"

class NodeUtil {
public:
    NodeUtil()                           = delete;
    NodeUtil(const NodeUtil&)            = delete;
    NodeUtil& operator=(const NodeUtil&) = delete;

    /// any nodes and attributes to be added
    static boost::python::object do_add(node_ptr self, const boost::python::object& arg);

    /// add ecflow variables from a python dictionary of strings
    static node_ptr add_variable_dict(node_ptr self, const boost::python::dict& dict);

    /// Add all the object in python list, to the node
    static boost::python::object node_iadd(node_ptr self, const boost::python::list& list);

    /// raw constructor,assumes first argument is a string.
    /// Assumes Task,Family,Suite has defined a constructor  init(const std::string& name, list attrs, dict kw)
    static boost::python::object node_raw_constructor(boost::python::tuple args, boost::python::dict kw);
};

#endif /* ecflow_python_NodeUtil_HPP */
