#ifndef EDIT_HPP_
#define EDIT_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $
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
#include "Variable.hpp"

class Edit {
public:
   explicit Edit(const boost::python::dict& dict);
   Edit(const boost::python::dict& dict,const boost::python::dict& dict2);
   const std::vector<Variable>& variables() const { return vec_;}
   static std::string to_string() { return "edit";}
   static boost::python::object init(boost::python::tuple args, boost::python::dict kw);
private:
   std::vector<Variable> vec_;
};

#endif
