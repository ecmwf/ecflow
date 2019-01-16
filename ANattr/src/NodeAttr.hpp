#ifndef NODEATTR_HPP_
#define NODEATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #61 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include <ostream>
#include <vector>
#include <limits> // for std::numeric_limits<int>::max()

#include <boost/operators.hpp>
#include <boost/utility.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/set.hpp>            // no need to include <set>
#include <boost/serialization/string.hpp>         // no need to include <string>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

////////////////////////////////////////////////////////////////////////////////////////
// Class Label:
// Use compiler , generated destructor, assignment,  copy constructor
class Label : public boost::equality_comparable<Label> {
public:
   Label(const std::string& name, const std::string& l);
   Label() : state_change_no_(0) {}

   std::ostream& print(std::ostream&) const;
   const std::string& name() const      { return name_;}
   const std::string& value() const     { return value_;}
   const std::string& new_value() const { return new_value_;}
   void set_new_value(const std::string& new_label);
   void reset();
   bool empty() const { return name_.empty(); }

   // The state_change_no is never reset. Must be incremented if it can affect equality
   unsigned int state_change_no() const { return state_change_no_; }

   // 2 kinds of equality, structure and state
   friend bool operator==(const Label& lhs,const Label& rhs) {
      if (lhs.name_ != rhs.name_ )  {
         //std::cout << "lhs.name_ '" << lhs.name_ << "' != rhs.name_ '" << rhs.name_ << "'\n";
         return false;
      }
      if (lhs.new_value_ != rhs.new_value_) {
         //std::cout << "lhs.new_value_ '" << lhs.new_value_ << "' != rhs.new_value_ '" << rhs.new_value_ << "'\n";
         return false;
      }
      if ( lhs.value_ != rhs.value_ ) {
         //std::cout << "lhs.value_ '" << lhs.value_ << "' != rhs.value_ '" << rhs.value_ << "'\n";
         return false;
      }
      return true;
   }

   std::string toString() const;
   std::string dump() const;

   void parse(const std::string& line, std::vector<std::string >& lineTokens, bool parse_state);
   static const Label& EMPTY(); // Added to support return by reference

private:
   std::string name_;
   std::string value_;
   std::string new_value_;
   unsigned int state_change_no_;        // *not* persisted, only used on server side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & name_;
      ar & value_;
      ar & new_value_;
   }
};


// Class Event:
// events with the number 007 are the same as 7.
// Use compiler , generated destructor, assignment, copy constructor
//
// Don't use -1, to represent that no number was specified, as on
// AIX portable binary archive can't cope with this
// use std::numeric_limits<int>::max()
class Event {
public:
   Event(int number, const std::string& eventName = "");
   explicit Event(const std::string& eventName);
   Event()
   : value_(false),
     number_(std::numeric_limits<int>::max()),
     used_(false),
     state_change_no_(0){}

   std::string name_or_number() const; // if name present return, else return number
   const std::string& name() const { return  name_;}
   std::ostream& print(std::ostream&) const;
   bool value() const { return value_;}
   void reset() { set_value(false);}
   bool empty() const { return (name_.empty() && number_ == std::numeric_limits<int>::max()); }

   int number() const { return number_;}
   bool operator==(const Event& rhs) const;
   void set_value(bool b);  // updates state_change_no_
   bool usedInTrigger() const { return used_;}
   void usedInTrigger(bool b) { used_ = b;}

   unsigned int state_change_no() const { return state_change_no_;}

   std::string toString() const;
   std::string dump() const;

   static bool isValidState(const std::string&); // return true for "set" | "clear"
   static const std::string& SET();
   static const std::string& CLEAR();
   static const Event& EMPTY(); // Added to support return by reference

private:
   bool         value_;
   int          number_;
   std::string  name_;
   bool         used_;             // used by the simulator not persisted
   unsigned int state_change_no_;  // *not* persisted, only used on server side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & value_;
      ar & number_;
      ar & name_;
   }
};

// Class Meter:
// Use compiler , generated destructor, assignment, copy constructor
// For this class we don't check the value member for the equality functionality
// Can have negative min/max however max >= min, and color change should be in the
// range min-max
class Meter {
public:
   Meter(const std::string& name,int min, int max, int colorChange = std::numeric_limits<int>::max());
   Meter() : min_(0),max_(0), value_(0),colorChange_(0),used_(false), state_change_no_(0){}

   std::ostream& print(std::ostream&) const;
   void reset() { set_value(min_);}
   void set_value(int v); // can throw throw std::runtime_error if out of range
   bool empty() const { return name_.empty(); }

   const std::string& name() const { return  name_;}
   int value() const { return value_;}
   int min() const { return min_;}
   int max() const { return max_;}
   int colorChange() const { return colorChange_;}

   // The state_change_no is never reset. Must be incremented if it can affect equality
   unsigned int state_change_no() const { return state_change_no_; }

   bool operator==(const Meter& rhs) const;

   bool usedInTrigger() const { return used_;}
   void usedInTrigger(bool b) { used_ = b;}
   std::string toString() const;
   std::string dump() const;

   static const Meter& EMPTY(); // Added to support return by reference

private:

   bool isValidValue(int v) const { return (v >= min_ && v <= max_); }

   int          min_;
   int          max_;
   int          value_;
   int          colorChange_;
   std::string  name_;
   bool         used_;        // used by the simulator not persisted
   unsigned int state_change_no_;  // *not* persisted, only used on server side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      ar & min_;
      ar & max_;
      ar & value_;
      ar & colorChange_;
      ar & name_;
   }
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(Meter, boost::serialization::object_serializable);
BOOST_CLASS_IMPLEMENTATION(Event, boost::serialization::object_serializable);
BOOST_CLASS_IMPLEMENTATION(Label, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(Meter,boost::serialization::track_never);
BOOST_CLASS_TRACKING(Event,boost::serialization::track_never);
BOOST_CLASS_TRACKING(Label,boost::serialization::track_never);
#endif
