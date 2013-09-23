//// Commented out since we *dont* use embedded python
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
//#include <boost/python.hpp>
//#include <boost/detail/lightweight_test.hpp>
//#include "boost/filesystem/operations.hpp"
//#include "boost/filesystem/path.hpp"
//#include <iostream>
//
//namespace fs     = boost::filesystem;
//namespace python = boost::python;
//
//// init_defs is defined in BOOST_PYTHON_MODULE(_defs) in file EcfDefsExt.cpp
//// However we need definition here. Hence expanded the pertinent contents of
//// BOOST_PYTHON_MODULE
//extern "C" __attribute__ ((visibility("default"))) void initecflow();
//
//void exec_test()
//{
//	std::cout << "registering extension module ecflow..." << std::endl;
//
//	// Register the module with the interpreter
//	if (PyImport_AppendInittab((char*)"_defs", initecflow) == -1)
//		throw std::runtime_error("Failed to add _defs to the interpreter's built in modules");
//
//	std::cout << "defining Defs..." << std::endl;
//
//	// Retrieve the main module
//	python::object main = python::import("__main__");
//
//	// Retrieve the main module's namespace
//	python::object global(main.attr("__dict__"));
//
//	// Define Defs in Python.
//	python::object result = python::exec(
//	                                     "from ecflow import *\n"
//	                                     "file = 'embedded.def'\n"
//	                                     "defs = Defs(file)\n"
//	                                     "suite = Suite('s1')\n"
//	                                     "family = Family('f1')\n"
//	                                     "for i in [ '_1', '_2', '_3' ]: family.add_task( Task( 't' + i) ) \n"
//	                                     "defs.add_suite(suite);  \n"
//	                                     "suite.add_family(family)\n"
//	                                     "defs.save_as_defs("embedded.def")\n",
// 	                                     global, global);
//
//	// Check it worked by looking for the presence of embedded.def file
// 	BOOST_TEST(fs::exists("embedded.def") );
// 	fs::remove("embedded.def");
//
//	std::cout << "success!" << std::endl;
//}
//
//int main(int argc, char **argv)
//{
//	// Initialize the interpreter
//	Py_Initialize();
//
//	if ( python::handle_exception(exec_test) )
//	{
//		if (PyErr_Occurred())
//		{
// 			PyErr_Print();
//		}
//		else
//		{
//			BOOST_ERROR("A C++ exception was thrown  for which "
//			            "there was no exception translator registered.");
//		}
//	}
//
//	// Boost.Python doesn't support Py_Finalize yet, so don't call it!
//	return boost::report_errors();
//}
