/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/python/DefsDoc.hpp"
#include "ecflow/python/NodeUtil.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"

namespace {

// NodeContainer

///
/// @brief Add a family to the node container and return it.
///
/// @param self The node container (Suite or Family) to add the family to.
/// @param f The family to add.
/// @return The added family.
///
node_ptr NodeContainer_add_family(NodeContainer* self, family_ptr f) {
    self->addFamily(f);
    return f;
}

///
/// @brief Create and add a new family with the given name to the node container.
///
/// @param self The node container (Suite or Family).
/// @param name The name of the new family.
/// @return The newly created family node.
///
node_ptr NodeContainer_add_family_by_name(NodeContainer* self, const std::string& name) {
    return self->add_family(name);
}

///
/// @brief Add a task to the node container and return it.
///
/// @param self The node container (Suite or Family) to add the task to.
/// @param t The task to add.
/// @return The added task.
///
node_ptr NodeContainer_add_task(NodeContainer* self, task_ptr t) {
    self->addTask(t);
    return t;
}

///
/// @brief Create and add a new task with the given name to the node container.
///
/// @param self The node container (Suite or Family).
/// @param name The name of the new task.
/// @return The newly created task node.
///
node_ptr NodeContainer_add_task_by_name(NodeContainer* self, const std::string& name) {
    return self->add_task(name);
}

// Suite

///
/// @brief Construct a Suite with the given name and optional attributes.
///
/// @param name The suite name.
/// @param args Positional attributes to add to the suite (forwarded to `NodeUtil::add`).
/// @param kwargs Keyword arguments added as variables on the suite.
/// @return The newly created Suite.
///
suite_ptr Suite_init(const std::string& name, const py::args& args, const py::kwargs& kwargs) {
    auto node = Suite::create(name);
    NodeUtil::add(*node, args);
    NodeUtil::add(*node, kwargs);
    return node;
}

///
/// @brief Return a string representation of the suite.
///
/// @param self The suite.
/// @return The suite serialised using the current print style.
///
std::string Suite_str(suite_ptr self) {
    return ecf::as_string(*self, PrintStyleHolder::getStyle());
}

///
/// @brief Implement the Python context manager entry protocol.
///
/// @param self The suite.
/// @return \p self, enabling use in a `with` statement.
///
suite_ptr Suite_enter(suite_ptr self) {
    return self;
}

///
/// @brief Implement the Python context manager exit protocol.
///
/// @return false (exceptions are not suppressed).
///
bool Suite_exit(suite_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

///
/// @brief Return the number of immediate child nodes in the suite.
///
/// @param self The suite.
/// @return The number of child nodes.
///
size_t Suite_len(suite_ptr self) {
    return self->nodeVec().size();
}

///
/// @brief Return true if the suite has an immediate child node with the given name.
///
/// @param self The suite.
/// @param name The name of the child node to search for.
/// @return true if a matching child exists, false otherwise.
///
bool Suite_contains(suite_ptr self, const std::string& name) {
    size_t pos;
    return (self->findImmediateChild(name, pos)) ? true : false;
}

///
/// @brief Add a clock attribute to the suite.
///
/// @param self The suite.
/// @param clk The clock attribute to add.
/// @return The suite (for method chaining).
///
suite_ptr Suite_add_clock(suite_ptr self, const ClockAttr& clk) {
    self->addClock(clk);
    return self;
}

///
/// @brief Add an end-clock attribute to the suite.
///
/// The end-clock marks the end of a simulation period.
///
/// @param self The suite.
/// @param clk The end-clock attribute to add.
/// @return The suite (for method chaining).
///
suite_ptr Suite_add_end_clock(suite_ptr self, const ClockAttr& clk) {
    self->add_end_clock(clk);
    return self;
}

// Family

///
/// @brief Construct a Family with the given name and optional attributes.
///
/// @param name The family name.
/// @param args Positional attributes to add to the family (forwarded to `NodeUtil::add`).
/// @param kwargs Keyword arguments added as variables on the family.
/// @return The newly created Family.
///
family_ptr Family_init(const std::string& name, const py::args& args, const py::kwargs& kwargs) {
    auto node = Family::create(name);
    NodeUtil::add(*node, args);
    NodeUtil::add(*node, kwargs);
    return node;
}

///
/// @brief Return a string representation of the family.
///
/// @param self The family.
/// @return The family serialised using the current print style.
///
std::string Family_str(family_ptr self) {
    return ecf::as_string(*self, PrintStyleHolder::getStyle());
}

///
/// @brief Implement the Python context manager entry protocol.
///
/// @param self The family.
/// @return \p self, enabling use in a `with` statement.
///
family_ptr Family_enter(family_ptr self) {
    return self;
}

///
/// @brief Implement the Python context manager exit protocol.
///
/// @return false (exceptions are not suppressed).
///
bool Family_exit(family_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

///
/// @brief Return the number of immediate child nodes in the family.
///
/// @param self The family.
/// @return The number of child nodes.
///
size_t Family_len(family_ptr self) {
    return self->nodeVec().size();
}

///
/// @brief Return true if the family has an immediate child node with the given name.
///
/// @param self The family.
/// @param name The name of the child node to search for.
/// @return true if a matching child exists, false otherwise.
///
bool Family_contains(family_ptr self, const std::string& name) {
    size_t pos;
    return (self->findImmediateChild(name, pos)) ? true : false;
}

} // namespace

void export_SuiteAndFamily(py::module& m) {

    py::class_<NodeContainer, Node, std::shared_ptr<NodeContainer>>(m, "NodeContainer", DefsDoc::node_container_doc())

        .def(
            "__iter__",
            [](const NodeContainer& n) { return py::make_iterator(n.nodeVec().begin(), n.nodeVec().end()); },
            py::keep_alive<0, 1>())
        .def("add_family", &NodeContainer_add_family_by_name, DefsDoc::add_family_doc())
        .def("add_family", NodeContainer_add_family)
        .def("add_task", &NodeContainer_add_task_by_name, DefsDoc::add_task_doc())
        .def("add_task", NodeContainer_add_task)
        .def("find_node", &NodeContainer::find_by_name, "Find immediate child node given a name")
        .def("find_task", &NodeContainer::findTask, "Find a task given a name")
        .def("find_family", &NodeContainer::findFamily, "Find a family given a name")
        .def_property_readonly("nodes", &NodeContainer::nodeVec, "Returns a list of Node's");

    py::class_<Suite, NodeContainer, std::shared_ptr<Suite>>(m, "Suite", DefsDoc::suite_doc())

        .def(py::init(&Suite_init), DefsDoc::suite_doc())
        .def(py::init<std::string, bool>(), py::arg("name"), py::arg("check") = true, DefsDoc::suite_doc())
        .def(py::self == py::self)
        .def("__str__", &Suite_str)
        .def("__copy__", pyutil_copy_object<Suite>)
        .def("__enter__", &Suite_enter)
        .def("__exit__", &Suite_exit)
        .def("__len__", &Suite_len)
        .def("__contains__", &Suite_contains)
        .def("add_clock", &Suite_add_clock)
        .def("get_clock", &Suite::clockAttr, "Returns the `suite`_ `clock`_")
        .def("add_end_clock", &Suite_add_end_clock, "End clock, used to mark end of simulation")
        .def("get_end_clock", &Suite::clock_end_attr, "Return the suite's end clock. Can be NULL")
        .def("begun", &Suite::begun, "Returns true if the `suite`_ has begun, false otherwise");

    py::class_<Family, NodeContainer, std::shared_ptr<Family>>(m, "Family")

        .def(py::init(&Family_init), DefsDoc::family_doc())
        .def(py::init<std::string, bool>(), py::arg("name"), py::arg("check") = true, DefsDoc::family_doc())
        .def(py::self == py::self)
        .def("__str__", &Family_str)
        .def("__copy__", pyutil_copy_object<Family>)
        .def("__enter__", &Family_enter)
        .def("__exit__", &Family_exit)
        .def("__len__", &Family_len)
        .def("__contains__", &Family_contains);
}
