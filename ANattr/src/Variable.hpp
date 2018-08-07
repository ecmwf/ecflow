#ifndef VARIABLE_HPP_
#define VARIABLE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #56 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <ostream>
#include "Serialization.hpp"

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
   : n_(name), v_(value) {}
   Variable(const std::string& name, const std::string& value);
   Variable() = default;

   const std::string& name() const   { return  n_;}
   std::ostream& print(std::ostream&) const;
   std::ostream& print_server_variable(std::ostream&) const;
   std::ostream& print_generated(std::ostream&) const;
   bool empty() const { return n_.empty(); }

   void set_value(const std::string& v) { v_ = v; }
   const std::string& theValue() const  { return  v_;}
   int value() const;

   void set_name(const std::string& v);
   std::string& value_by_ref() { return v_;}

   bool operator==(const Variable& rhs) const;
   std::string toString() const;
   std::string dump() const;

   // Added to support return by reference
   static const Variable& EMPTY();

private:
   std::string  n_;
   std::string  v_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar)
   {
      ar( CEREAL_NVP(n_),
          CEREAL_NVP(v_)
      );
   }
};

#endif
