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

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

node_ptr add_family_by_name(NodeContainer* self, const std::string& name) {
    return self->add_family(name);
}

node_ptr add_family(NodeContainer* self, family_ptr f) {
    self->addFamily(f);
    return f;
}
node_ptr add_task(NodeContainer* self, task_ptr t) {
    self->addTask(t);
    std::cout << "add_task: " << t->name() << ", and has parent: " << (t->parent() ? "true" : "false") << std::endl;
    return t;
}

node_ptr add_task_by_name(NodeContainer* self, const std::string& name) {
    return self->add_task(name);
}

suite_ptr add_clock(suite_ptr self, const ClockAttr& clk) {
    self->addClock(clk);
    return self;
}
suite_ptr add_end_clock(suite_ptr self, const ClockAttr& clk) {
    self->add_end_clock(clk);
    return self;
}

// Sized and Container protocol
size_t family_len(family_ptr self) {
    return self->nodeVec().size();
}
size_t suite_len(suite_ptr self) {
    return self->nodeVec().size();
}
bool family_container(family_ptr self, const std::string& name) {
    size_t pos;
    return (self->findImmediateChild(name, pos)) ? true : false;
}
bool suite_container(suite_ptr self, const std::string& name) {
    size_t pos;
    return (self->findImmediateChild(name, pos)) ? true : false;
}

// Context management, Only used to provide indentation

std::string suite_to_string(suite_ptr self) {
    return ecf::as_string(*self, PrintStyleHolder::getStyle());
}

suite_ptr suite_enter(suite_ptr self) {
    return self;
}

bool suite_exit(suite_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

std::string family_to_string(family_ptr self) {
    return ecf::as_string(*self, PrintStyleHolder::getStyle());
}

family_ptr family_enter(family_ptr self) {
    return self;
}

bool family_exit(family_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

family_ptr family_init(const std::string& name, const py::args& args, const py::kwargs& kwargs) {
    auto node = Family::create(name);
    NodeUtil::add(*node, args);
    NodeUtil::add(*node, kwargs);
    return node;
}

suite_ptr suite_init(const std::string& name, const py::args& args, const py::kwargs& kwargs) {
    auto node = Suite::create(name);
    NodeUtil::add(*node, args);
    NodeUtil::add(*node, kwargs);
    return node;
}

void export_SuiteAndFamily(py::module& m) {

    py::class_<NodeContainer, Node, std::shared_ptr<NodeContainer>>(m, "NodeContainer", DefsDoc::node_container_doc())

        .def(
            "__iter__",
            [](const NodeContainer& n) { return py::make_iterator(n.nodeVec().begin(), n.nodeVec().end()); },
            py::keep_alive<0, 1>())
        .def("add_family", &add_family_by_name, DefsDoc::add_family_doc())
        .def("add_family", add_family)
        .def("add_task", &add_task_by_name, DefsDoc::add_task_doc())
        .def("add_task", add_task)
        .def("find_node", &NodeContainer::find_by_name, "Find immediate child node given a name")
        .def("find_task", &NodeContainer::findTask, "Find a task given a name")
        .def("find_family", &NodeContainer::findFamily, "Find a family given a name")
        .def_property_readonly("nodes", &NodeContainer::nodeVec, "Returns a list of Node's");

    py::class_<Family, NodeContainer, std::shared_ptr<Family>>(m, "Family")

        .def(py::init(&family_init), DefsDoc::family_doc())
        .def(py::init<std::string, bool>(), py::arg("name"), py::arg("check") = true, DefsDoc::family_doc())
        .def(py::self == py::self)                   // __eq__
        .def("__str__", &family_to_string)           // __str__
        .def("__copy__", pyutil_copy_object<Family>) // __copy__ uses copy constructor
        .def("__enter__", &family_enter)             // allow with statement, hence indentation support
        .def("__exit__", &family_exit)               // allow with statement, hence indentation support
        .def("__len__", &family_len)                 // Implement sized protocol for immediate children
        .def("__contains__", &family_container);     // Implement container protocol for immediate children

#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<family_ptr>(); // needed for mac and boost 1.6
#endif

    py::class_<Suite, NodeContainer, std::shared_ptr<Suite>>(m, "Suite", DefsDoc::suite_doc())

        .def(py::init(&suite_init), DefsDoc::suite_doc())
        .def(py::init<std::string, bool>(), py::arg("name"), py::arg("check") = true, DefsDoc::suite_doc())
        .def(py::self == py::self)                  // __eq__
        .def("__str__", &suite_to_string)           // __str__
        .def("__copy__", pyutil_copy_object<Suite>) // __copy__ uses copy constructor
        .def("__enter__", &suite_enter)             // allow with statement, hence indentation support
        .def("__exit__", &suite_exit)               // allow with statement, hence indentation support
        .def("__len__", &suite_len)                 // Implement sized protocol for immediate children
        .def("__contains__", &suite_container)      // Implement container protocol for immediate children
        .def("add_clock", &add_clock)
        .def("get_clock", &Suite::clockAttr, "Returns the `suite`_ `clock`_")
        .def("add_end_clock", &add_end_clock, "End clock, used to mark end of simulation")
        .def("get_end_clock", &Suite::clock_end_attr, "Return the suite's end clock. Can be NULL")
        .def("begun", &Suite::begun, "Returns true if the `suite`_ has begun, false otherwise");

#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<suite_ptr>(); // needed for mac and boost 1.6
#endif
}
