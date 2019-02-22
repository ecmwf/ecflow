#ifndef GENERIC_ATTR_HPP_
#define GENERIC_ATTR_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
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
#include <iosfwd>
#include <vector>
#include <cereal/access.hpp>

// Class GenericAttr:
// Use compiler , generated destructor, assignment, copy constructor
// GenericAttr does *not* have any changeable state
class GenericAttr {
public:
   GenericAttr(const std::string& name, const std::vector<std::string>& values);
   GenericAttr(const std::string& name);
   GenericAttr() = default;

   bool operator==(const GenericAttr& rhs) const;
   void print(std::string&) const;
   bool empty() const { return name_.empty(); }

   const std::string& name() const { return name_;}
   const std::vector<std::string>& values() { return values_;}

   std::vector<std::string>::const_iterator values_begin() const { return values_.begin();} // for python
   std::vector<std::string>::const_iterator values_end()   const { return values_.end();  } // for python

   std::string to_string() const;

   // Added to support return by reference
   static const GenericAttr& EMPTY();

private:
   std::string name_;
   std::vector<std::string> values_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar);
};

#endif
