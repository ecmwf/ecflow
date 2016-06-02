#ifndef BOOST_PYTHON_UTIL_HPP_
#define BOOST_PYTHON_UTIL_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include <boost/python.hpp>
#include <vector>
#include <string>

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

class BoostPythonUtil : private boost::noncopyable {
public:

   /// Convert python list to a vector of integers. raises a type error if integer extraction fails
   static void list_to_int_vec(const boost::python::list& list, std::vector<int>& int_vec);
   static void list_to_str_vec(const boost::python::list& list, std::vector<std::string>& int_vec);
   static void dict_to_str_vec(const boost::python::dict& dict, std::vector<std::pair<std::string,std::string> >& str_pair);

};

#endif
