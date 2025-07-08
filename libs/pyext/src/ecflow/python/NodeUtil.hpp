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

#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/python/PythonBinding.hpp"

class NodeUtil {
public:
    NodeUtil()                           = delete;
    NodeUtil(const NodeUtil&)            = delete;
    NodeUtil& operator=(const NodeUtil&) = delete;

    /// Any nodes and attributes to be added
    static bp::object do_add(node_ptr self, const bp::object& arg);

    /// Add ecflow variables from a python dictionary of strings
    static node_ptr add_variable_dict(node_ptr self, const bp::dict& dict);

    /// Add all the object in a python list, to the node
    static bp::object node_iadd(node_ptr self, const bp::list& list);

    /// The raw constructor assumes the first argument is a string.
    /// Assumes Task, Family, or Suite has defined a constructor  init(const std::string& name, list attrs, dict kw)
    static bp::object node_raw_constructor(bp::tuple args, bp::dict kw);
};

#endif /* ecflow_python_NodeUtil_HPP */
