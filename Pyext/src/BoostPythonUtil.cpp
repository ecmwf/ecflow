
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "BoostPythonUtil.hpp"

void BoostPythonUtil::list_to_int_vec(const boost::python::list& list, std::vector<int>& int_vec)
{
   int the_list_size = len(list);
   int_vec.reserve(the_list_size);
   for (int i = 0; i < the_list_size; ++i) {
      int_vec.push_back(boost::python::extract<int>(list[i]));
   }
}

void BoostPythonUtil::list_to_str_vec(const boost::python::list& list, std::vector<std::string>& vec)
{
   int the_list_size = len(list);
   vec.reserve(the_list_size);
   for (int i = 0; i < the_list_size; ++i) {
      vec.push_back(boost::python::extract<std::string>(list[i]));
   }
}

void BoostPythonUtil::dict_to_str_vec(const boost::python::dict& dict, std::vector<std::pair<std::string,std::string> >& str_pair_vec)
{
   boost::python::list keys = dict.keys();
   const int no_of_keys = len(keys);
   str_pair_vec.reserve(no_of_keys);

   for(int i = 0; i < no_of_keys; ++i) {

      std::string second;
      std::string first = boost::python::extract<std::string>(keys[i]);
      if (boost::python::extract<std::string>(dict[keys[i]]).check()) {
         second = boost::python::extract<std::string>(dict[keys[i]]);
      }
      else if (boost::python::extract<int>(dict[keys[i]]).check()) {
         int the_int = boost::python::extract<int>(dict[keys[i]]);
         second = boost::lexical_cast<std::string>(the_int);
      }
      else throw std::runtime_error("BoostPythonUtil::dict_to_str_vec: type not convertible to string or integer");
      str_pair_vec.push_back( std::make_pair(first,second));
      //         std::cout << "BoostPythonUtil::dict_to_str_vec " << first << "   " << second << "\n";
   }
}


void BoostPythonUtil::dict_to_str_vec(const boost::python::dict& dict, std::vector<Variable>& vec)
{
   boost::python::list keys = dict.keys();
   const int no_of_keys = len(keys);
   vec.reserve(no_of_keys);

   for(int i = 0; i < no_of_keys; ++i) {

      std::string second;
      std::string first = boost::python::extract<std::string>(keys[i]);
      if (boost::python::extract<std::string>(dict[keys[i]]).check()) {
         second = boost::python::extract<std::string>(dict[keys[i]]);
      }
      else if (boost::python::extract<int>(dict[keys[i]]).check()) {
         int the_int = boost::python::extract<int>(dict[keys[i]]);
         second = boost::lexical_cast<std::string>(the_int);
      }
      else throw std::runtime_error("BoostPythonUtil::dict_to_str_vec: type not convertible to string or integer");

      vec.push_back(Variable(first,second));
      // std::cout << "BoostPythonUtil::dict_to_str_vec " << first << "   " << second << "\n";
   }
}

