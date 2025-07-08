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
#include <boost/python/raw_function.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/python/BoostPythonUtil.hpp"
#include "ecflow/python/DefsDoc.hpp"
#include "ecflow/python/NodeUtil.hpp"

namespace bp = boost::python;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

/// Since we don't pass in a child pos, the nodes are added to the end
family_ptr add_family(NodeContainer* self, family_ptr f) {
    self->addFamily(f);
    return f;
}
task_ptr add_task(NodeContainer* self, task_ptr t) {
    self->addTask(t);
    return t;
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

bool suite_exit(suite_ptr self, const bp::object& type, const bp::object& value, const bp::object& traceback) {
    return false;
}

std::string family_to_string(family_ptr self) {
    return ecf::as_string(*self, PrintStyleHolder::getStyle());
}

family_ptr family_enter(family_ptr self) {
    return self;
}

bool family_exit(family_ptr self, const bp::object& type, const bp::object& value, const bp::object& traceback) {
    return false;
}

family_ptr family_init(const std::string& name, bp::list the_list, bp::dict kw) {
    // cout << "family_init : " << name << " the_list: " << len(the_list) << " dict: " << len(kw) << endl;
    family_ptr node = Family::create(name);
    (void)NodeUtil::add_variable_dict(node, kw);
    (void)NodeUtil::node_iadd(node, the_list);
    return node;
}

suite_ptr suite_init(const std::string& name, bp::list the_list, bp::dict kw) {
    // cout << "suite_init : " << name << " the_list: " << len(the_list) << " dict: " << len(kw) << endl;
    suite_ptr node = Suite::create(name);
    (void)NodeUtil::add_variable_dict(node, kw);
    (void)NodeUtil::node_iadd(node, the_list);
    return node;
}

void export_SuiteAndFamily() {
    // Turn off proxies by passing true as the NoProxy template parameter.
    // shared_ptrs don't need proxies because calls on one a copy of the
    // shared_ptr will affect all of them (duh!).
    bp::class_<std::vector<family_ptr>>("FamilyVec", "Hold a list of `family`_ nodes")
        .def(bp::vector_indexing_suite<std::vector<family_ptr>, true>());

    bp::class_<std::vector<suite_ptr>>("SuiteVec", "Hold a list of `suite`_ nodes's")
        .def(bp::vector_indexing_suite<std::vector<suite_ptr>, true>());

    // choose the correct overload
    bp::class_<NodeContainer, bp::bases<Node>, boost::noncopyable>(
        "NodeContainer", DefsDoc::node_container_doc(), bp::no_init)
        .def("__iter__", bp::range(&NodeContainer::node_begin, &NodeContainer::node_end))
        .def("add_family", &NodeContainer::add_family, DefsDoc::add_family_doc())
        .def("add_family", add_family)
        .def("add_task", &NodeContainer::add_task, DefsDoc::add_task_doc())
        .def("add_task", add_task)
        .def("find_node", &NodeContainer::find_by_name, "Find immediate child node given a name")
        .def("find_task", &NodeContainer::findTask, "Find a task given a name")
        .def("find_family", &NodeContainer::findFamily, "Find a family given a name")
        .add_property(
            "nodes", bp::range(&NodeContainer::node_begin, &NodeContainer::node_end), "Returns a list of Node's");

    bp::class_<Family, bp::bases<NodeContainer>, family_ptr>("Family", DefsDoc::family_doc())
        .def("__init__", bp::raw_function(&NodeUtil::node_raw_constructor, 1)) // will call -> family_init
        .def("__init__", bp::make_constructor(&family_init), DefsDoc::family_doc())
        .def("__init__", bp::make_constructor(&Family::create_me), DefsDoc::family_doc())
        .def(bp::self == bp::self)              // __eq__
        .def("__str__", &family_to_string)      // __str__
        .def("__copy__", copyObject<Family>)    // __copy__ uses copy constructor
        .def("__enter__", &family_enter)        // allow with statement, hence indentation support
        .def("__exit__", &family_exit)          // allow with statement, hence indentation support
        .def("__len__", &family_len)            // Implement sized protocol for immediate children
        .def("__contains__", &family_container) // Implement container protocol for immediate children
        ;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    bp::register_ptr_to_python<family_ptr>(); // needed for mac and boost 1.6
#endif

    bp::class_<Suite, bp::bases<NodeContainer>, suite_ptr>("Suite", DefsDoc::suite_doc())
        .def("__init__", bp::raw_function(&NodeUtil::node_raw_constructor, 1)) // will call -> suite_init
        .def("__init__", bp::make_constructor(&suite_init), DefsDoc::suite_doc())
        .def("__init__", bp::make_constructor(&Suite::create_me), DefsDoc::suite_doc())
        .def(bp::self == bp::self)             // __eq__
        .def("__str__", &suite_to_string)      // __str__
        .def("__copy__", copyObject<Suite>)    // __copy__ uses copy constructor
        .def("__enter__", &suite_enter)        // allow with statement, hence indentation support
        .def("__exit__", &suite_exit)          // allow with statement, hence indentation support
        .def("__len__", &suite_len)            // Implement sized protocol for immediate children
        .def("__contains__", &suite_container) // Implement container protocol for immediate children
        .def("add_clock", &add_clock)
        .def("get_clock", &Suite::clockAttr, "Returns the `suite`_ `clock`_")
        .def("add_end_clock", &add_end_clock, "End clock, used to mark end of simulation")
        .def("get_end_clock", &Suite::clock_end_attr, "Return the suite's end clock. Can be NULL")
        .def("begun", &Suite::begun, "Returns true if the `suite`_ has begun, false otherwise");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    bp::register_ptr_to_python<suite_ptr>(); // needed for mac and boost 1.6
#endif
}
