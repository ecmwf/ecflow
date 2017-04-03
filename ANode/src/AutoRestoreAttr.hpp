#ifndef AutoRestoreAttr_HPP_
#define AutoRestoreAttr_HPP_
//============================================================================
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
//============================================================================

#include "NodeFwd.hpp"

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor
class AutoRestoreAttr  {
public:
   AutoRestoreAttr(Node* n) : node_(n) {}
   AutoRestoreAttr(const AutoRestoreAttr& rhs) : node_(NULL),nodes_to_restore_(rhs.nodes_to_restore_)  {}
   AutoRestoreAttr() : node_(NULL) {}

   // needed by node copy constructor and persistence
   void set_node(Node* n) { node_ = n; }

   std::ostream& print(std::ostream&) const;
   bool operator==(const AutoRestoreAttr& rhs) const;

   std::string toString() const;

   void do_auto_restore();
   void add_nodes_to_restore( const std::vector<std::string>& nr ) { nodes_to_restore_ = nr;}
   std::vector<std::string>& nodes_to_restore() const { return nodes_to_restore; }
   bool check(std::string& errorMsg) const; // check auto restore can reference the nodes

private:
   Node* node_;                                // Not persisted, constructor will always set this up.
   std::vector<std::string> nodes_to_restore_; // must be suite or family

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & nodes_to_restore_;
   }
};

}
#endif
