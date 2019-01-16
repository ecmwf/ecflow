/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $
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
#include "BoostPythonUtil.hpp"
#include "Edit.hpp"

using namespace boost::python;
using namespace std;
namespace bp = boost::python;

Edit::Edit(const boost::python::dict& dict){BoostPythonUtil::dict_to_str_vec(dict,vec_);}
Edit::Edit(const boost::python::dict& dict,const boost::python::dict& dict2){BoostPythonUtil::dict_to_str_vec(dict,vec_);BoostPythonUtil::dict_to_str_vec(dict2,vec_);}

object Edit::init(boost::python::tuple args, dict kw) {
   //cout << "Edit::init args: " << len(args) << " kwargs " << len(kw) << "\n";
   // args[0] is Edit(i.e self)
   for (int i = 1; i < len(args) ; ++i) {
      if (boost::python::extract<dict>(args[i]).check()){
         dict d = boost::python::extract<dict>(args[i]);
         return args[0].attr("__init__")(d,kw); // calls -> .def(init<dict,dict>() -> Edit(dict,dict)
      }
      else throw std::runtime_error("Edit::Edit: only accepts dictionary and key word arguments");
   }
   bp::tuple rest(args.slice(1,_));
   return args[0].attr("__init__")(kw); // calls -> .def(init<dict>() -> Edit(const boost::python::dict& dict)
}
