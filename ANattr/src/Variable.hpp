#ifndef VARIABLE_HPP_
#define VARIABLE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #56 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <ostream>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/level.hpp>

////////////////////////////////////////////////////////////////////////////////////////
// Class Variable:
// Use compiler , generated destructor, assignment,  copy constructor
class Variable {
public:
   // This constructor added as an optimisation to avoid, checking variable names.
   // i.e the generated variables, and created by the default constructors of Suite/Family/Task etc
   // These are called during serialisation, hence to avoid checking generated names, we know are valid
   // use this constructor. The bool is used as a dummy argument, so that we call the right constructor
   Variable(const std::string& name, const std::string& value, bool /*check_names_dummy*/)
   : name_(name), value_(value) {}
   Variable(const std::string& name, const std::string& value);
   Variable() {}

   const std::string& name() const   { return  name_;}
   std::ostream& print(std::ostream&) const;
   bool empty() const { return name_.empty(); }

   void set_value(const std::string& v) { value_ = v; }
   const std::string& theValue() const  { return  value_;}
   int value() const;

   void set_name(const std::string& v) { name_ = v; }
   std::string& value_by_ref() { return value_;}

   bool operator==(const Variable& rhs) const;
   std::string toString() const;
   std::string dump() const;

   // Added to support return by reference
   static const Variable& EMPTY();

private:
   std::string  name_;
   std::string  value_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & name_;
      ar & value_;
   }
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(Variable, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(Variable,boost::serialization::track_never);

#endif
