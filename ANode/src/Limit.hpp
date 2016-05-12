#ifndef LIMIT_HPP_
#define LIMIT_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #61 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description : Limit was placed in the ANode category because inlimit
//               can reference Limit on *ANOTHER* suite. This presents
//               A problem with incremental sync, since that requires
//               access to a parent/suite, to mark the suite as changed.
//               To get round this issue the Node will set the
//               parent pointer on the Limit this then makes it easy
//               for incremental sync, since we directly access the parent suite
//============================================================================

#include <ostream>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>            // no need to include <set>
#include <boost/serialization/string.hpp>         // no need to include <string>
class Node;

// Class Limit: The limit is zero based, hence if limit is 10, increment must use < 10
class Limit {
public:
   Limit(const std::string& name,int limit);
   Limit(const std::string& name,int limit, int value, const std::set<std::string>& paths);
   Limit() : state_change_no_(0), theLimit_(0), value_(0),node_(0)  {}
   Limit(const Limit& rhs);

   std::ostream& print(std::ostream&) const;
   bool operator==(const Limit& rhs) const;
   const std::string& name() const { return  name_;}

   void set_node(Node* n) { node_ = n; }

   void setValue(int v);
   void setLimit(int v);
   void set_state(int limit, int value,const std::set<std::string>& p); // for use by memento
   void set_paths(const std::set<std::string>& p);

   bool delete_path( const std::string& abs_node_path); // for use by AlterCmd
   const std::set<std::string>& paths() const { return paths_;}

   int value() const { return value_;}
   bool inLimit(int inlimit_tokens) const { return ((value_ + inlimit_tokens) <= theLimit_);}
   int theLimit() const { return theLimit_;}
   void increment(int tokens, const std::string& abs_node_path);
   void decrement(int tokens, const std::string& abs_node_path);
   void reset();

   // The state_change_no is never reset. Must be incremented if it can affect equality
   unsigned int state_change_no() const { return state_change_no_; }

   // for python interface
   std::string toString() const;

   // ECFLOW-518, we can't use:
   //    std::set<std::string>::const_iterator paths_begin() const { return paths_.begin();}
   //    std::set<std::string>::const_iterator paths_end() const { return paths_.end();}
   // because boost python does not support std::set<std::string> out of the box
   // we will wrap and return list instead. See ExportNodeAttr.cpp

private:
   void update_change_no();

private:
   unsigned int             state_change_no_;  // *not* persisted, only used on server side
   std::string              name_;
   int                      theLimit_;
   int                      value_;
   std::set<std::string>    paths_;           // Updated via increment()/decrement()/reset(). Typically task paths
   Node*                    node_ ;           // The parent NOT persisted

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & name_;
      ar & theLimit_;
      ar & value_;
      ar & paths_;
   }
};

#endif
