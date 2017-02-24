#ifndef QUEUE_ATTR_HPP_
#define QUEUE_ATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #51 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <ostream>
#include "boost_archive.hpp" // collates boost archive includes

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>         // no need to include <vector>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/export.hpp>   // explicit code for exports (place last) , needed for BOOST_CLASS_EXPORT

#include <boost/utility.hpp>

/////////////////////////////////////////////////////////////////////////
class QueueAttr {
public:
   QueueAttr(const std::string& name,const std::vector<std::string>& theQueue);
   QueueAttr() : used_in_trigger_(false),currentIndex_(0),state_change_no_(0) {}
   ~QueueAttr();

   std::ostream& print(std::ostream&) const;
   bool operator==(const QueueAttr& rhs) const;

   /// Accessor
   const std::string& name() const { return name_;}
   std::string value() const;
   int index_or_value() const;
   int index() const { return currentIndex_;}
   bool empty() const { return name_.empty();}
   const std::vector<std::string>& list() const { return theQueue_;}

   std::string toString() const;
   std::string dump() const;

   // Mutators
   void reset();
   void increment();
   void set_used_in_trigger(bool f) { used_in_trigger_ = f;} // used by simulator only
   bool used_in_trigger() const { return used_in_trigger_; }

   static void parse( QueueAttr&, const std::string& line, std::vector<std::string >& lineTokens, bool parse_state );
   void set_name( const std::string& name);
   void set_queue( const std::vector<std::string>& theQueue);
   void set_index( int index) { currentIndex_ = index;}

   // Added to support return by reference
   static const QueueAttr& EMPTY();
   static QueueAttr& EMPTY1();

   unsigned int state_change_no() const { return state_change_no_;}
private:
   void incr_state_change_no();

   bool used_in_trigger_;      // *not* persisted, used by simulator only
   int currentIndex_;
   unsigned int state_change_no_;  // *not* persisted, only used on server side
   std::string name_;
   std::vector<std::string> theQueue_;

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & currentIndex_;
      ar & name_;
      ar & theQueue_;
   }
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(QueueAttr, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(QueueAttr,boost::serialization::track_never);

#endif
