/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #93 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/raw_function.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "PrintStyle.hpp"
#include "File.hpp"
#include "JobCreationCtrl.hpp"
#include "Simulator.hpp"
#include "BoostPythonUtil.hpp"

#include "DefsDoc.hpp"

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

void save_as_checkpt(defs_ptr defs, const std::string& file_name)
{
   defs->boost_save_as_checkpt(file_name); // use default ARCHIVE
}

void restore_from_checkpt(defs_ptr defs, const std::string& file_name)
{
   // Temp, until default ecflow version is 4.7.0, ECFLOW-939
   try {
      defs->restore(file_name);
      return;
   }
   catch(...){}

   defs->boost_restore_from_checkpt(file_name); // use default ARCHIVE
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
bool defs_exit(defs_ptr self,const boost::python::object& type,const boost::python::object& value,const boost::python::object& traceback){return false;}

std::string check_job_creation(defs_ptr defs)
{
   job_creation_ctrl_ptr jobCtrl = boost::make_shared<JobCreationCtrl>();
   defs->check_job_creation(jobCtrl);
   return jobCtrl->get_error_msg();
}

// Add  server user variables
defs_ptr add_variable(defs_ptr self,const std::string& name, const std::string& value) {
   self->set_server().add_or_update_user_variables(name,value); return self;}
defs_ptr add_variable_int(defs_ptr self,const std::string& name, int value) {
   self->set_server().add_or_update_user_variables(name, boost::lexical_cast<std::string>(value)); return self;}
defs_ptr add_variable_var(defs_ptr self,const Variable& var) {
   self->set_server().add_or_update_user_variables(var.name(),var.theValue()); return self;}
defs_ptr add_variable_dict(defs_ptr self,const boost::python::dict& dict) {
   std::vector<std::pair<std::string,std::string> > vec;
   BoostPythonUtil::dict_to_str_vec(dict,vec);
   std::vector<std::pair<std::string,std::string> >::iterator i;
   std::vector<std::pair<std::string,std::string> >::iterator vec_end = vec.end();
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

static object add(tuple args, dict kwargs) {
   int the_list_size = len(args);
   defs_ptr self = boost::python::extract<defs_ptr>(args[0]); // self
   if (!self) throw std::runtime_error("ExportDefs::add() : first argument is not a defs");

   for (int i = 1; i < the_list_size; ++i) {
      if (boost::python::extract<Variable>(args[i]).check()) {
         Variable var = boost::python::extract<Variable>(args[i]);
         self->set_server().add_or_update_user_variables(var.name(),var.theValue());
      }
      else if (boost::python::extract<dict>(args[i]).check())      add_variable_dict(self,boost::python::extract<dict>(args[i]) );
      else if (boost::python::extract<suite_ptr>(args[i]).check()) self->addSuite(boost::python::extract<suite_ptr>(args[i])) ;
      else throw std::runtime_error("ExportDefs::add : Unknown type");
   }

   boost::python::list keys =  kwargs.keys();
   const int no_of_keys = len(keys);
   for(int i = 0; i < no_of_keys; ++i) {
      boost::python::object curArg = keys[i];
      if (curArg) {
         std::string first = boost::python::extract<std::string>(keys[i]);
         std::string second = boost::python::extract<std::string>(kwargs[keys[i]]);
         self->set_server().add_or_update_user_variables(first,second);
      }
   }
   return object(self); // return defs as python object, relies class_<Defs>... for type registration
}

void export_Defs()
{
	class_<Defs,defs_ptr >( "Defs", DefsDoc::add_definition_doc() ,init<>("Create a empty Defs"))
   .def("__init__",make_constructor(&create_defs),         DefsDoc::add_definition_doc())
	.def(self == self )                                           // __eq__
   .def("__copy__",              copyObject<Defs>)               // __copy__ uses copy constructor
	.def("__str__",               &Defs::toString)                // __str__
   .def("__enter__",             &defs_enter)                    // allow with statement, hence indentation support
   .def("__exit__",              &defs_exit)                     // allow with statement, hence indentation support
   .def("add", raw_function(add,1))
   .def("add_suite",             &add_suite,               DefsDoc::add_suite_doc())
   .def("add_suite",             &Defs::add_suite )
 	.def("add_extern",            &Defs::add_extern,        DefsDoc::add_extern_doc())
	.def("auto_add_externs",      &Defs::auto_add_externs,  DefsDoc::add_extern_doc())
   .def("add_variable",          &add_variable,            DefsDoc::add_variable_doc())
   .def("add_variable",          &add_variable_int)
   .def("add_variable",          &add_variable_var)
   .def("add_variable",          &add_variable_dict)
   .def("sort_attributes",       &sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
   .def("sort_attributes",       &Defs::sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
   .def("delete_variable",       &delete_variable,"An empty string will delete all user variables")
	.def("find_suite",            &Defs::findSuite,"Given a name, find the corresponding :term:`suite`")
   .def("find_abs_node",         &Defs::findAbsNode,"Given a path, find the the :term:`node`")
   .def("get_all_nodes",         &get_all_nodes,"Returns all the :term:`node` s in the definition")
   .def("get_all_tasks",         &get_all_tasks,"Returns all the :term:`task` nodes")
	.def("has_time_dependencies", &Defs::hasTimeDependencies,"returns True if the :term:`suite definition` has any time :term:`dependencies`")
	.def("save_as_checkpt",       &save_as_checkpt, "Save the in memory :term:`suite definition` as a :term:`check point` file. This includes all node state.")
	.def("restore_from_checkpt",  &restore_from_checkpt, "Restore the :term:`suite definition` from a :term:`check point` file stored on disk")
   .def("save_as_defs",          &save_as_defs,   "Save the in memory :term:`suite definition` into a file. The file name must be passed as an argument\n\n")
   .def("save_as_defs",          &save_as_defs_1, "Save the in memory :term:`suite definition` into a file. The file name must be passed as an argument\n\n")
	.def("check",                 &check_defs,               DefsDoc::check())
	.def("simulate",              &simulate,                 DefsDoc::simulate())
   .def("check_job_creation",    &check_job_creation,       DefsDoc::check_job_creation_doc() )
   .def("check_job_creation",    &Defs::check_job_creation)
   .def("generate_scripts",      &Defs::generate_scripts,   DefsDoc::generate_scripts_doc() )
   .def("get_state",             &Defs::state )
   .def("get_server_state",      &get_server_state,         DefsDoc::get_server_state() )
	.add_property("suites",       boost::python::range( &Defs::suite_begin, &Defs::suite_end),"Returns a list of :term:`suite` s")
	.add_property("externs",      boost::python::range( &Defs::extern_begin, &Defs::extern_end),"Returns a list of :term:`extern` s" )
   .add_property("user_variables", boost::python::range( &Defs::user_variables_begin, &Defs::user_variables_end),"Returns a list of user defined :term:`variable` s" )
   .add_property("server_variables", boost::python::range( &Defs::server_variables_begin, &Defs::server_variables_end),"Returns a list of server :term:`variable` s" )
	;
}
