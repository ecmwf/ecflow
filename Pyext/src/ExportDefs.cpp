/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #93 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/noncopyable.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "PrintStyle.hpp"
#include "File.hpp"
#include "JobCreationCtrl.hpp"
#include "Simulator.hpp"

#include "BoostPythonUtil.hpp"
#include "Edit.hpp"
#include "DefsDoc.hpp"
#include "GlossaryDoc.hpp"

using namespace ecf;
using namespace boost::python;
using namespace std;
namespace bp = boost::python;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

void save_as_defs(const Defs& theDefs, const std::string& filename, PrintStyle::Type_t the_style_enum)
{
   PrintStyle style(the_style_enum);
   std::stringstream ss; ss << theDefs;

   std::string file_creation_error_msg;
   if (!File::create(filename,ss.str(),file_creation_error_msg)) {
      std::string error = "save_as_defs failed: ";
      error += file_creation_error_msg;
      throw std::runtime_error(error);
   }
}

void save_as_defs_1(const Defs& theDefs, const std::string& filename)
{
   save_as_defs(theDefs,filename,PrintStyle::DEFS);
}

static defs_ptr create_defs(const std::string& file_name)
{
   defs_ptr defs = Defs::create();

   std::string errorMsg,warningMsg;
   if (!defs->restore(file_name,errorMsg,warningMsg)) {
      throw std::runtime_error(errorMsg);
   }
   if (!warningMsg.empty()) std::cerr << warningMsg;
   return defs;
}

std::string check_defs(defs_ptr defs)
{
	std::string  error_msg;
	std::string  warning_msg;
 	if (defs.get() && !defs->check(error_msg,warning_msg)) {
 		error_msg += "\n";
 		error_msg += warning_msg;
 		return error_msg;
 	}
 	return warning_msg;
}

void restore_from_checkpt(defs_ptr defs, const std::string& file_name)
{
   defs->restore(file_name);
}

std::string simulate(defs_ptr defs)
{
   if (defs.get()) {
      // name output file after name of the first suite
      std::string defs_filename = "pyext.def";
      if (!defs->suiteVec().empty()) {
         defs_filename = (*defs->suiteVec().begin())->name() + ".def";
      }

      Simulator simulator;
      std::string errorMsg;
      if (!simulator.run(*defs, defs_filename, errorMsg)) {
         return errorMsg;
      }
   }
	return string();
}

SState::State get_server_state(defs_ptr self) { return self->server().get_state(); }

/// Since we don't pass in a child pos, the nodes are added to the end
suite_ptr add_suite(defs_ptr self,suite_ptr s){ self->addSuite(s); return s; }

std::vector<task_ptr> get_all_tasks(defs_ptr self){ std::vector<task_ptr> tasks; self->get_all_tasks(tasks); return tasks; }
std::vector<node_ptr> get_all_nodes(defs_ptr self){ std::vector<node_ptr> nodes; self->get_all_nodes(nodes); return nodes; }

// Context management, Only used to provide indentation
defs_ptr defs_enter(defs_ptr self) { return self;}
bool defs_exit(defs_ptr self,const bp::object& type,const bp::object& value,const bp::object& traceback){return false;}

std::string check_job_creation(defs_ptr defs, bool throw_on_error, bool verbose){
   job_creation_ctrl_ptr jobCtrl = std::make_shared<JobCreationCtrl>();
   if (verbose) jobCtrl->set_verbose(verbose);
   defs->check_job_creation(jobCtrl);
   if (!jobCtrl->get_error_msg().empty() && throw_on_error) {
      throw std::runtime_error(jobCtrl->get_error_msg());
   }
   return jobCtrl->get_error_msg();
}

// Add  server user variables
defs_ptr add_variable(defs_ptr self,const std::string& name, const std::string& value) {
   self->set_server().add_or_update_user_variables(name,value); return self;}
defs_ptr add_variable_int(defs_ptr self,const std::string& name, int value) {
   self->set_server().add_or_update_user_variables(name, boost::lexical_cast<std::string>(value)); return self;}
defs_ptr add_variable_var(defs_ptr self,const Variable& var) {
   self->set_server().add_or_update_user_variables(var.name(),var.theValue()); return self;}
defs_ptr add_variable_dict(defs_ptr self,const bp::dict& dict) {
   std::vector<std::pair<std::string,std::string> > vec;
   BoostPythonUtil::dict_to_str_vec(dict,vec);
   std::vector<std::pair<std::string,std::string> >::iterator i;
   auto vec_end = vec.end();
   for(i = vec.begin(); i != vec_end; ++i) {
      self->set_server().add_or_update_user_variables((*i).first,(*i).second);
   }
   return self;
}
void delete_variable(defs_ptr self,const std::string& name) { self->set_server().delete_user_variable(name);}

void sort_attributes(defs_ptr self,const std::string& attribute_name, bool recursive){
   std::string attribute = attribute_name; boost::algorithm::to_lower(attribute);
   ecf::Attr::Type attr = Attr::to_attr(attribute_name);
   if (attr == ecf::Attr::UNKNOWN) {
      std::stringstream ss;  ss << "sort_attributes: the attribute " << attribute_name << " is not valid";
      throw std::runtime_error(ss.str());
   }
   self->sort_attributes(attr,recursive);
}

// Support sized and Container protocol
size_t defs_len(defs_ptr self) { return self->suiteVec().size();}
bool defs_container(defs_ptr self, const std::string& name){return (self->findSuite(name)) ?  true : false;}

static object do_add(defs_ptr self, const bp::object& arg) {
   //std::cout << "defs::do_add \n";
   if (arg.ptr() == object().ptr())  return object(self); // *IGNORE* None
   else if (extract<suite_ptr>(arg).check()) self->addSuite(extract<suite_ptr>(arg)) ;
   else if (extract<dict>(arg).check())     add_variable_dict(self,extract<dict>(arg));
   else if (extract<Edit>(arg).check()) {
      Edit edit = extract<Edit>(arg);
      const std::vector<Variable>& vec = edit.variables();
      for(const auto & i : vec) self->set_server().add_or_update_user_variables(i.name(),i.theValue());
   }
   else if (extract<bp::list>(arg).check()){
      bp::list the_list = extract<bp::list>(arg);
      int the_list_size = len(the_list);
      for(int i = 0; i < the_list_size; ++i) (void) do_add(self,the_list[i]); // recursive
   }
   else if (extract<Variable>(arg).check()) {
      Variable var = extract<Variable>(arg);
      self->set_server().add_or_update_user_variables(var.name(),var.theValue());
   }
   else throw std::runtime_error("ExportDefs::add : Unknown type");
   return object(self);
}

static object add(bp::tuple args, dict kwargs) {
   int the_list_size = len(args);
   defs_ptr self = extract<defs_ptr>(args[0]); // self
   if (!self) throw std::runtime_error("ExportDefs::add() : first argument is not a Defs");

   for (int i = 1; i < the_list_size; ++i) (void)do_add(self,args[i]);
   (void)add_variable_dict(self,kwargs);

   return object(self); // return defs as python object, relies class_<Defs>... for type registration
}

static object defs_iadd(defs_ptr self, const bp::list& list) {
   //std::cout << "defs_iadd  list " << self->name() << "\n";
   int the_list_size = len(list);
   for(int i = 0; i < the_list_size; ++i) (void)do_add(self,list[i]);
   return object(self); // return node_ptr as python object, relies class_<Node>... for type registration
}

static object defs_getattr(defs_ptr self, const std::string& attr) {
   // cout << "  defs_getattr  self.name() : " << self->name() << "  attr " << attr << "\n";
   suite_ptr child = self->findSuite(attr);
   if (child) return object(child);

   Variable var = self->server().findVariable(attr);
   if (!var.empty()) return object(var);

   std::stringstream ss; ss << "ExportDefs::defs_getattr : function of name '" << attr << "' does not exist *OR* suite or defs variable";
   throw std::runtime_error(ss.str());
   return object();
}

object defs_raw_constructor(bp::tuple args, dict kw) {
   // cout << "defs_raw_constructor  len(args):" << len(args) << endl;
   // args[0] is Defs(i.e self)
   bp::list the_list;
   std::string name;
   for (int i = 1; i < len(args) ; ++i) {
      if (extract<string>(args[i]).check()) name = extract<string>(args[i]);
      else the_list.append(args[i]);
   }
   if (!name.empty() && len(the_list) > 0)
      throw std::runtime_error("defs_raw_constructor: Can't mix string with other arguments. String argument specifies a path(loads a definition from disk)");
   return args[0].attr("__init__")(the_list,kw); // calls -> init(list attr, dict kw)
}

defs_ptr defs_init( bp::list the_list, bp::dict kw) {
   // cout << " defs_init: the_list: " << len(the_list) << " dict: " << len(kw) << endl;
   defs_ptr defs = Defs::create();
   (void) add_variable_dict(defs,kw);
   (void) defs_iadd(defs,the_list);
   return defs;
}

void export_Defs()
{
	class_<Defs,defs_ptr>( "Defs", DefsDoc::add_definition_doc(),init<>("Create a empty Defs"))
   .def("__init__",raw_function(&defs_raw_constructor,0))  // will call -> task_init
   .def("__init__",make_constructor(&defs_init))
   .def("__init__",make_constructor(&create_defs),         DefsDoc::add_definition_doc())
	.def(self == self )                                           // __eq__
	.def("__copy__",              copyObject<Defs>)               // __copy__ uses copy constructor
	.def("__str__",               &Defs::toString)                // __str__
	.def("__enter__",             &defs_enter)                    // allow with statement, hence indentation support
	.def("__exit__",              &defs_exit)                     // allow with statement, hence indentation support
	.def("__len__",               &defs_len)                      // Sized protocol
	.def("__contains__",          &defs_container)                // Container protocol
	.def("__iter__",              bp::range(&Defs::suite_begin, &Defs::suite_end)) // iterable protocol
	.def("__getattr__",           &defs_getattr) /* Any attempt to resolve a property, method, or field name that doesn't actually exist on the object itself will be passed to __getattr__*/
   .def("__iadd__",              &defs_iadd)  // defs += [ Suite('s1'), Edit(var='value'), Variable('a','b') [ Suite('t2') ] ]
   .def("__iadd__",              &do_add)     // defs += Suite("s1")
   .def("__add__",               &do_add)
	.def("add",                   raw_function(add,1),DefsDoc::add())
	.def("add_suite",             &add_suite,               DefsDoc::add_suite_doc())
	.def("add_suite",             &Defs::add_suite, GlossaryDoc::list() )
	.def("add_extern",            &Defs::add_extern,        DefsDoc::add_extern_doc())
	.def("auto_add_externs",      &Defs::auto_add_externs,  DefsDoc::add_extern_doc())
	.def("add_variable",          &add_variable,            DefsDoc::add_variable_doc())
	.def("add_variable",          &add_variable_int)
	.def("add_variable",          &add_variable_var)
	.def("add_variable",          &add_variable_dict)
	.def("sort_attributes",       &sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
	.def("sort_attributes",       &Defs::sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
	.def("delete_variable",       &delete_variable,"An empty string will delete all user variables")
	.def("find_suite",            &Defs::findSuite,"Given a name, find the corresponding `suite`_")
	.def("find_abs_node",         &Defs::findAbsNode,"Given a path, find the the `node`_")
	.def("get_all_nodes",         &get_all_nodes,"Returns all the `node`_ s in the definition")
	.def("get_all_tasks",         &get_all_tasks,"Returns all the `task`_ nodes")
	.def("has_time_dependencies", &Defs::hasTimeDependencies,"returns True if the `suite definition`_ has any time `dependencies`_")
	.def("save_as_checkpt",       &Defs::save_as_checkpt, "Save the in memory `suite definition`_ as a `check point`_ file. This includes all node state.")
	.def("restore_from_checkpt",  &restore_from_checkpt, "Restore the `suite definition`_ from a `check point`_ file stored on disk")
	.def("save_as_defs",          &save_as_defs,   "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")
	.def("save_as_defs",          &save_as_defs_1, "Save the in memory `suite definition`_ into a file. The file name must be passed as an argument\n\n")
	.def("check",                 &check_defs,               DefsDoc::check())
	.def("simulate",              &simulate,                 DefsDoc::simulate())
	.def("check_job_creation",    &check_job_creation,(bp::arg("throw_on_error")=false,bp::arg("verbose")=false),DefsDoc::check_job_creation_doc() )
	.def("check_job_creation",    &Defs::check_job_creation)
	.def("generate_scripts",      &Defs::generate_scripts,   DefsDoc::generate_scripts_doc() )
	.def("get_state",             &Defs::state )
	.def("get_server_state",      &get_server_state,         DefsDoc::get_server_state() )
	.add_property("suites",       bp::range( &Defs::suite_begin, &Defs::suite_end),"Returns a list of `suite`_ s")
	.add_property("externs",      bp::range( &Defs::extern_begin, &Defs::extern_end),"Returns a list of `extern`_ s" )
	.add_property("user_variables", bp::range( &Defs::user_variables_begin, &Defs::user_variables_end),"Returns a list of user defined `variable`_ s" )
	.add_property("server_variables", bp::range( &Defs::server_variables_begin, &Defs::server_variables_end),"Returns a list of server `variable`_ s" )
	;

#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python<defs_ptr>(); // needed for mac and boost 1.6
#endif
}
