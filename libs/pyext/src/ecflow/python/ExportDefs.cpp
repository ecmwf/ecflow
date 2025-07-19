/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/python/DefsDoc.hpp"
#include "ecflow/python/Edit.hpp"
#include "ecflow/python/ExportCollections.hpp"
#include "ecflow/python/GlossaryDoc.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"
#include "ecflow/simulator/Simulator.hpp"

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

void save_as_defs(const Defs& theDefs, const std::string& filename, PrintStyle::Type_t the_style_enum) {
    std::string file_creation_error_msg;
    if (!ecf::File::create(filename, ecf::as_string(theDefs, the_style_enum), file_creation_error_msg)) {
        std::string error = "save_as_defs failed: ";
        error += file_creation_error_msg;
        throw std::runtime_error(error);
    }
}

void save_as_defs_1(const Defs& theDefs, const std::string& filename) {
    save_as_defs(theDefs, filename, PrintStyle::DEFS);
}

std::string convert_to_string(const Defs& theDefs) {
    std::string buffer;
    ecf::write_t(buffer, theDefs, PrintStyleHolder::getStyle());
    return buffer;
}

std::string check_defs(defs_ptr defs) {
    std::string error_msg;
    std::string warning_msg;
    if (defs.get() && !defs->check(error_msg, warning_msg)) {
        error_msg += "\n";
        error_msg += warning_msg;
        return error_msg;
    }
    return warning_msg;
}

void restore_from_checkpt(defs_ptr defs, const std::string& file_name) {
    defs->restore(file_name);
}

std::string simulate(defs_ptr defs) {
    if (defs.get()) {
        // name output file after name of the first suite
        std::string defs_filename = "pyext.def";
        if (!defs->suiteVec().empty()) {
            defs_filename = (*defs->suiteVec().begin())->name() + ".def";
        }

        ecf::Simulator simulator;
        std::string errorMsg;
        if (!simulator.run(*defs, defs_filename, errorMsg)) {
            return errorMsg;
        }
    }
    return std::string();
}

SState::State get_server_state(defs_ptr self) {
    return self->server_state().get_state();
}

/// Since we don't pass in a child pos, the nodes are added to the end
suite_ptr add_suite(defs_ptr self, suite_ptr s) {
    self->addSuite(s);
    return s;
}

std::vector<task_ptr> get_all_tasks(defs_ptr self) {
    return ecf::get_all_tasks_ptr(*self);
}

std::vector<node_ptr> get_all_nodes(defs_ptr self) {
    return ecf::get_all_nodes_ptr(*self);
}

// Context management, Only used to provide indentation
defs_ptr defs_enter(defs_ptr self) {
    return self;
}
bool defs_exit(defs_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

std::string check_job_creation(defs_ptr defs, bool throw_on_error, bool verbose) {
    job_creation_ctrl_ptr jobCtrl = std::make_shared<JobCreationCtrl>();
    if (verbose) {
        jobCtrl->set_verbose(verbose);
    }
    defs->check_job_creation(jobCtrl);
    if (!jobCtrl->get_error_msg().empty() && throw_on_error) {
        throw std::runtime_error(jobCtrl->get_error_msg());
    }
    return jobCtrl->get_error_msg();
}

// Add  server user variables
defs_ptr add_variable(defs_ptr self, const std::string& name, const std::string& value) {
    self->server_state().add_or_update_user_variables(name, value);
    return self;
}
defs_ptr add_variable_int(defs_ptr self, const std::string& name, int value) {
    self->server_state().add_or_update_user_variables(name, ecf::convert_to<std::string>(value));
    return self;
}
defs_ptr add_variable_var(defs_ptr self, const Variable& var) {
    self->server_state().add_or_update_user_variables(var.name(), var.theValue());
    return self;
}
defs_ptr add_variable_dict(defs_ptr self, const py::dict& dict) {
    std::vector<std::pair<std::string, std::string>> vec;
    pyutil_dict_to_str_vec(dict, vec);
    std::vector<std::pair<std::string, std::string>>::iterator i;
    auto vec_end = vec.end();
    for (i = vec.begin(); i != vec_end; ++i) {
        self->server_state().add_or_update_user_variables((*i).first, (*i).second);
    }
    return self;
}
void delete_variable(defs_ptr self, const std::string& name) {
    self->server_state().delete_user_variable(name);
}

void sort_attributes(defs_ptr self, ecf::Attr::Type attr) {
    self->sort_attributes(attr);
}
void sort_attributes1(defs_ptr self, ecf::Attr::Type attr, bool recurse) {
    self->sort_attributes(attr, recurse);
}
void sort_attributes2(defs_ptr self, ecf::Attr::Type attr, bool recurse, const py::list& list) {
    std::vector<std::string> no_sort;
    pyutil_list_to_str_vec(list, no_sort);
    self->sort_attributes(attr, recurse, no_sort);
}

void sort_attributes3(defs_ptr self, const std::string& attribute_name, bool recursive, const py::list& list) {
    std::string attribute = attribute_name;
    boost::algorithm::to_lower(attribute);
    ecf::Attr::Type attr = ecf::Attr::to_attr(attribute_name);
    if (attr == ecf::Attr::UNKNOWN) {
        throw std::runtime_error(MESSAGE("sort_attributes: the attribute " << attribute_name << " is not valid"));
    }
    std::vector<std::string> no_sort;
    pyutil_list_to_str_vec(list, no_sort);
    self->sort_attributes(attr, recursive, no_sort);
}

// Support sized and Container protocol
size_t defs_len(defs_ptr self) {
    return self->suiteVec().size();
}
bool defs_container(defs_ptr self, const std::string& name) {
    return (self->findSuite(name)) ? true : false;
}

static py::object do_add(defs_ptr self, const py::handle& arg) {
    // When arg is None, there is nothing to do...
    if (arg == py::none()) {
        return py::cast(self);
    }

    if (auto found = py_extract<suite_ptr>(arg); found) {
        self->addSuite(found.value());
    }
    else if (auto found = py_extract<Suite>(arg); found) {
        auto suite = std::make_shared<Suite>(found.value());
        self->addSuite(suite);
    }
    else if (auto found = py_extract<py::dict>(arg); found) {
        add_variable_dict(self, found.value());
    }
    else if (auto found = py_extract<Edit>(arg); found) {
        Edit edit = found.value();

        const std::vector<Variable>& vec = edit.variables();
        for (const auto& i : vec) {
            self->server_state().add_or_update_user_variables(i.name(), i.theValue());
        }
    }
    else if (auto found = py_extract<py::list>(arg); found) {
        py::list the_list = found.value();

        int the_list_size = len(the_list);
        for (int i = 0; i < the_list_size; ++i) {
            (void)do_add(self, the_list[i]); // recursive
        }
    }
    else if (auto found = py_extract<Variable>(arg); found) {
        Variable var = found.value();
        self->server_state().add_or_update_user_variables(var.name(), var.theValue());
    }
    else {
        throw std::runtime_error("ExportDefs::add : Unknown type");
    }
    return py::cast(self);
}

static py::object defs_add(defs_ptr self, const py::args& args, const py::kwargs& kwargs) {
    for (auto arg : args) {
        do_add(self, arg);
    }
    add_variable_dict(self, kwargs);

    return py::cast(self); // return defs as python object, relies class_<Defs>... for type registration
}

static py::object defs_iadd(defs_ptr self, const py::list& list) {
    // std::cout << "defs_iadd  list " << self->name() << "\n";
    int the_list_size = len(list);
    for (int i = 0; i < the_list_size; ++i) {
        (void)do_add(self, list[i]);
    }
    return py::cast(self); // return defs_ptr as python object, relies class_<Defs>... for type registration
}

static py::object defs_getattr(defs_ptr self, const std::string& attr) {
    std::cout << "  defs_getattr  self.name() : " << self->name() << "  attr " << attr << "\n";
    suite_ptr child = self->findSuite(attr);
    if (child) {
        return py::cast(child);
    }

    Variable var = self->server_state().findVariable(attr);
    if (!var.empty()) {
        return py::cast(var);
    }

    throw std::runtime_error(MESSAGE("ExportDefs::defs_getattr : function of name '"
                                     << attr << "' does not exist *OR* suite or defs variable"));
}

static defs_ptr defs_create(const std::string& filename) {
    defs_ptr defs = Defs::create();

    std::string errorMsg, warningMsg;
    if (!defs->restore(filename, errorMsg, warningMsg)) {
        throw std::runtime_error(errorMsg);
    }
    if (!warningMsg.empty()) {
        std::cerr << warningMsg;
    }
    return defs;
}

static defs_ptr defs_init(const py::args& args, const py::kwargs& kw) {
    defs_ptr defs = Defs::create();
    defs_add(defs, args, kw);
    return defs;
}

void export_Defs(py::module& m) {

    py::class_<Defs, defs_ptr>(m, "Defs", py::dynamic_attr())

        .def(py::init<>(), "Create an empty Defs")
        .def(py::init(&defs_create), DefsDoc::add_definition_doc())
        .def(py::init(&defs_init))
        .def(py::self == py::self)                 // __eq__
        .def("__copy__", pyutil_copy_object<Defs>) // __copy__ uses copy constructor
        .def("__str__", convert_to_string)         // __str__
        .def("__enter__", &defs_enter)             // allow with statement, hence indentation support
        .def("__exit__", &defs_exit)               // allow with statement, hence indentation support
        .def("__len__", &defs_len)                 // Sized protocol
        .def("__contains__", &defs_container)      // Container protocol
        .def(
            "__iter__",
            [](const Defs& s) { return py::make_iterator(s.suiteVec().begin(), s.suiteVec().end()); },
            py::keep_alive<0, 1>())
        .def("__getattr__", &defs_getattr) /* Any attempt to resolve a property, method, or field name that doesn't
                                              actually exist on the object itself will be passed to __getattr__*/
        .def("__iadd__", &defs_iadd) // defs += [ Suite('s1'), Edit(var='value'), Variable('a','b') [ Suite('t2') ] ]
        .def("__iadd__", &do_add)    // defs += Suite("s1")
        .def("__add__", &do_add)
        .def("add", &defs_add, DefsDoc::add())
        .def("add_suite", &add_suite, DefsDoc::add_suite_doc())
        .def("add_suite", &Defs::add_suite, GlossaryDoc::list())
        .def("add_extern", &Defs::add_extern, DefsDoc::add_extern_doc())
        .def("auto_add_externs", &Defs::auto_add_externs, DefsDoc::add_extern_doc())
        .def("add_variable", &add_variable, DefsDoc::add_variable_doc())
        .def("add_variable", &add_variable_int)
        .def("add_variable", &add_variable_var)
        .def("add_variable", &add_variable_dict)
        .def("sort_attributes", &sort_attributes)
        .def("sort_attributes", &sort_attributes1)
        .def("sort_attributes", &sort_attributes2)
        .def("sort_attributes",
             &sort_attributes3,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())
        .def("sort_attributes",
             &Defs::sort_attributes,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())
        .def("delete_variable", &delete_variable, "An empty string will delete all user variables")
        .def("find_suite", &Defs::findSuite, "Given a name, find the corresponding `suite`_")
        .def("find_abs_node", &Defs::findAbsNode, "Given a path, find the the `node`_")
        .def("find_node_path",
             &Defs::find_node_path,
             "Given a type(suite,family,task) and a name, return path of the first match, otherwise return an empty "
             "string")
        .def("find_node", &Defs::find_node, "Given a type(suite,family,task) and a path to a node, return the node.")
        .def("get_all_nodes", &get_all_nodes, "Returns all the `node`_\\ s in the definition")
        .def("get_all_tasks", &get_all_tasks, "Returns all the `task`_ nodes")
        .def("has_time_dependencies",
             &Defs::hasTimeDependencies,
             "returns True if the `suite definition`_ has any time `dependencies`_")
        .def("save_as_checkpt",
             &Defs::write_to_checkpt_file,
             "Save the in memory `suite definition`_ as a `check point`_ file. This includes all node state.")
        .def("restore_from_checkpt",
             &restore_from_checkpt,
             "Restore the `suite definition`_ from a `check point`_ file stored on disk")
        .def("save_as_defs",
             &save_as_defs,
             "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")
        .def("save_as_defs",
             &save_as_defs_1,
             "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")
        .def("check", &check_defs, DefsDoc::check())
        .def("simulate", &simulate, DefsDoc::simulate())
        .def("check_job_creation",
             &check_job_creation,
             py::arg("throw_on_error") = false,
             py::arg("verbose")        = false,
             DefsDoc::check_job_creation_doc())
        .def("check_job_creation", &Defs::check_job_creation)
        .def("generate_scripts", &Defs::generate_scripts, DefsDoc::generate_scripts_doc())
        .def("get_state", &Defs::state)
        .def("get_server_state", &get_server_state, DefsDoc::get_server_state())
        .def_property_readonly(
            "suites",
            [](const Defs& s) { return py::make_iterator(s.suiteVec().begin(), s.suiteVec().end()); },
            py::keep_alive<0, 1>(),
            "Returns a list of `suite`_\\ s")
        .def_property_readonly("externs", &Defs::externs, "Returns a list of `extern`_\\ s")
        .def_property_readonly(
            "user_variables", &Defs::user_variables, "Returns a list of user defined `variable`_\\ s")
        .def_property_readonly("server_variables", &Defs::server_variables, "Returns a list of server `variable`_\\ s")

        .doc() = DefsDoc::add_definition_doc();

#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<defs_ptr>(); // needed for mac and boost 1.6
#endif
}
