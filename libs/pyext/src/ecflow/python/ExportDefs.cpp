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

namespace {

void Defs_save_as_defs_with_given_style(const Defs& theDefs,
                                        const std::string& filename,
                                        PrintStyle::Type_t the_style_enum) {
    std::string file_creation_error_msg;
    if (!ecf::File::create(filename, ecf::as_string(theDefs, the_style_enum), file_creation_error_msg)) {
        std::string error = "save_as_defs failed: ";
        error += file_creation_error_msg;
        throw std::runtime_error(error);
    }
}

void Defs_save_as_defs_with_default_style(const Defs& theDefs, const std::string& filename) {
    Defs_save_as_defs_with_given_style(theDefs, filename, PrintStyle::DEFS);
}

std::string Defs_str(const Defs& theDefs) {
    std::string buffer;
    ecf::write_t(buffer, theDefs, PrintStyleHolder::getStyle());
    return buffer;
}

std::string Defs_check(defs_ptr defs) {
    std::string error_msg;
    std::string warning_msg;
    if (defs.get() && !defs->check(error_msg, warning_msg)) {
        error_msg += "\n";
        error_msg += warning_msg;
        return error_msg;
    }
    return warning_msg;
}

void Defs_restore(defs_ptr defs, const std::string& file_name) {
    defs->restore(file_name);
}

std::string Defs_simulate(defs_ptr defs) {
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

SState::State Defs_get_server_state(defs_ptr self) {
    return self->server_state().get_state();
}

suite_ptr Defs_add_suite(defs_ptr self, suite_ptr s) {
    // Since we don't pass in a child pos, the nodes are added to the end
    self->addSuite(s);
    return s;
}

std::vector<task_ptr> Defs_get_all_tasks(defs_ptr self) {
    return ecf::get_all_tasks_ptr(*self);
}
std::vector<node_ptr> Defs_get_all_nodes(defs_ptr self) {
    return ecf::get_all_nodes_ptr(*self);
}

defs_ptr Defs_enter(defs_ptr self) {
    return self;
}

bool Defs_exit(defs_ptr self, const py::object& type, const py::object& value, const py::object& traceback) {
    return false;
}

std::string Defs_check_job_creation(defs_ptr defs, bool throw_on_error, bool verbose) {
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

defs_ptr Defs_add_variable_string(defs_ptr self, const std::string& name, const std::string& value) {
    self->server_state().add_or_update_user_variables(name, value);
    return self;
}

defs_ptr Defs_add_variable_int(defs_ptr self, const std::string& name, int value) {
    self->server_state().add_or_update_user_variables(name, ecf::convert_to<std::string>(value));
    return self;
}

defs_ptr Defs_add_variable_variable(defs_ptr self, const Variable& var) {
    self->server_state().add_or_update_user_variables(var.name(), var.theValue());
    return self;
}

defs_ptr Defs_add_variable_dict(defs_ptr self, const py::dict& dict) {
    std::vector<std::pair<std::string, std::string>> vec;
    pyutil_dict_to_str_vec(dict, vec);
    std::vector<std::pair<std::string, std::string>>::iterator i;
    auto vec_end = vec.end();
    for (i = vec.begin(); i != vec_end; ++i) {
        self->server_state().add_or_update_user_variables((*i).first, (*i).second);
    }
    return self;
}

void Defs_delete_variable(defs_ptr self, const std::string& name) {
    self->server_state().delete_user_variable(name);
}

void Defs_sort_attributes(defs_ptr self, const std::string& attribute_name, bool recursive, const py::list& list) {
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

size_t Defs_len(defs_ptr self) {
    return self->suiteVec().size();
}

bool Defs_container(defs_ptr self, const std::string& name) {
    return (self->findSuite(name)) ? true : false;
}

py::object Defs_add(defs_ptr self, const py::handle& arg) {
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
        Defs_add_variable_dict(self, found.value());
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
            (void)Defs_add(self, the_list[i]); // recursive
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

py::object Defs_add_args_kwargs(defs_ptr self, const py::args& args, const py::kwargs& kwargs) {
    for (auto arg : args) {
        Defs_add(self, arg);
    }
    Defs_add_variable_dict(self, kwargs);

    return py::cast(self); // return defs as python object, relies class_<Defs>... for type registration
}

py::object Defs_iadd(defs_ptr self, const py::list& list) {
    int the_list_size = len(list);
    for (int i = 0; i < the_list_size; ++i) {
        (void)Defs_add(self, list[i]);
    }
    return py::cast(self); // return defs_ptr as python object, relies class_<Defs>... for type registration
}

py::object Defs_getattr(defs_ptr self, const std::string& attr) {
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

defs_ptr Defs_make(const std::string& filename) {
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

defs_ptr Defs_init(const py::args& args, const py::kwargs& kw) {
    defs_ptr defs = Defs::create();
    Defs_add_args_kwargs(defs, args, kw);
    return defs;
}

} // namespace

void export_Defs(py::module& m) {

    py::class_<Defs, defs_ptr>(m, "Defs", py::dynamic_attr(), DefsDoc::add_definition_doc())

        .def(py::init<>(), "Create an empty Defs")
        .def(py::init(&Defs_make), DefsDoc::add_definition_doc())
        .def(py::init(&Defs_init))

        .def(py::self == py::self)
        .def("__copy__", pyutil_copy_object<Defs>)
        .def("__str__", Defs_str)

        // *** Iteration ***

        .def(
            "__iter__",
            [](const Defs& s) { return py::make_iterator(s.suiteVec().begin(), s.suiteVec().end()); },
            py::keep_alive<0, 1>())
        .def("__len__", &Defs_len)
        .def("__contains__", &Defs_container)

        // *** Operators ***

        .def("__add__", &Defs_add)
        .def("__iadd__", &Defs_iadd) // defs += [ Suite('s1'), Edit(var='value'), Variable('a','b') [ Suite('t2') ] ]
        .def("__iadd__", &Defs_add)  // defs += Suite("s1")

        // *** Context Manager ***

        .def("__enter__", &Defs_enter) // allow with statement, hence indentation support
        .def("__exit__", &Defs_exit)   // allow with statement, hence indentation support

        // *** Dynamic Attributes ***

        .def("__getattr__", &Defs_getattr)

        .def("add", &Defs_add_args_kwargs, DefsDoc::add())

        .def("add_suite", &Defs_add_suite, DefsDoc::add_suite_doc())
        .def("add_suite", &Defs::add_suite, GlossaryDoc::list())

        .def("add_extern", &Defs::add_extern, DefsDoc::add_extern_doc())

        .def("auto_add_externs", &Defs::auto_add_externs, DefsDoc::add_extern_doc())

        .def("add_variable", &Defs_add_variable_string, DefsDoc::add_variable_doc())
        .def("add_variable", &Defs_add_variable_int)
        .def("add_variable", &Defs_add_variable_variable)
        .def("add_variable", &Defs_add_variable_dict)

        .def("sort_attributes",
             &Defs_sort_attributes,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())
        .def("sort_attributes",
             &Defs::sort_attributes,
             py::arg("attribute_type"),
             py::arg("recursive") = true,
             py::arg("no_sort")   = py::list())

        .def("delete_variable", &Defs_delete_variable, "An empty string will delete all user variables")

        .def("find_suite", &Defs::findSuite, "Given a name, find the corresponding `suite`_")

        .def("find_abs_node", &Defs::findAbsNode, "Given a path, find the the `node`_")

        .def("find_node_path",
             &Defs::find_node_path,
             "Given a type(suite,family,task) and a name, return path of the first match, otherwise return an empty "
             "string")

        .def("find_node", &Defs::find_node, "Given a type(suite,family,task) and a path to a node, return the node.")

        .def("get_all_nodes", &Defs_get_all_nodes, "Returns all the `node`_\\ s in the definition")

        .def("get_all_tasks", &Defs_get_all_tasks, "Returns all the `task`_ nodes")

        .def("has_time_dependencies",
             &Defs::hasTimeDependencies,
             "returns True if the `suite definition`_ has any time `dependencies`_")

        .def("save_as_checkpt",
             &Defs::write_to_checkpt_file,
             "Save the in memory `suite definition`_ as a `check point`_ file. This includes all node state.")

        .def("restore_from_checkpt",
             &Defs_restore,
             "Restore the `suite definition`_ from a `check point`_ file stored on disk")

        .def("save_as_defs",
             &Defs_save_as_defs_with_given_style,
             "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")

        .def("save_as_defs",
             &Defs_save_as_defs_with_default_style,
             "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")

        .def("check", &Defs_check, DefsDoc::check())

        .def("simulate", &Defs_simulate, DefsDoc::simulate())

        .def("check_job_creation",
             &Defs_check_job_creation,
             py::arg("throw_on_error") = false,
             py::arg("verbose")        = false,
             DefsDoc::check_job_creation_doc())

        .def("check_job_creation", &Defs::check_job_creation)

        .def("generate_scripts", &Defs::generate_scripts, DefsDoc::generate_scripts_doc())

        .def("get_state", &Defs::state)

        .def("get_server_state", &Defs_get_server_state, DefsDoc::get_server_state())

        .def_property_readonly(
            "suites",
            [](const Defs& s) { return py::make_iterator(s.suiteVec().begin(), s.suiteVec().end()); },
            py::keep_alive<0, 1>(),
            "Returns a list of `suite`_\\ s")

        .def_property_readonly("externs", &Defs::externs, "Returns a list of `extern`_\\ s")

        .def_property_readonly(
            "user_variables", &Defs::user_variables, "Returns a list of user defined `variable`_\\ s")

        .def_property_readonly("server_variables", &Defs::server_variables, "Returns a list of server `variable`_\\ s");
}
