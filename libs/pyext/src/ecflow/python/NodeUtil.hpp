/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_python_NodeUtil_HPP
#define ecflow_python_NodeUtil_HPP

#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/python/PythonBinding.hpp"

class NodeUtil {
public:
    NodeUtil() = delete;

    /// Add the attribute(s), provided as a python object, to the node
    static Node& add1(Node& self, const py::object& args);

    /// Add the attribute(s), provided as a python object, to the node
    static void add(Node& self, const py::handle& arg);

    /// Add the attribute(s), provided as python args, to the nodes
    static void add(Node& self, const py::args& args);
    /// Add the attribute(s), provided as python kwargs, to the node
    /// Since kwargs is a dictionary, each entry will be added as a Variable with name and value
    static void add(Node& self, const py::kwargs& kwargs);
};

#endif /* ecflow_python_NodeUtil_HPP */
