#ifndef REPEATATTR_HPP_
#define REPEATATTR_HPP_
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
//
// Description : Repeat Attribute. Please note that for repeat string, enumeration
//               the positional index is used for evaluation.
//
// Simulation: Simulation must not affect the real job submission in the server.
//  o Infinite repeats cause problems with simulation, hence we have
//    a mechanism to stop this, when reset is called, via server this is disabled
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

#include "Variable.hpp"

/////////////////////////////////////////////////////////////////////////
// Node can only have one repeat.
//
class RepeatBase {
public:
   RepeatBase(const std::string& name) : state_change_no_(0), name_(name) {}
   RepeatBase() : state_change_no_(0) {}
   virtual ~RepeatBase();

   /// make non virtual so that it can be in-lined. Called millions of times
   const std::string& name() const { return name_; }
   const Variable& gen_variable() const { return var_; }
   Variable& set_gen_variable() const { return var_; }

   virtual int start() const = 0;
   virtual int end() const = 0;
   virtual int step() const = 0;

   // After Repeat expiration the last call to increment() can cause
   // value to be beyond the last valid value
   // Depending on the kind of repeat the returned can be value or the current index
   // RepeatDate       -> value
   // RepeatString     -> index into array of strings
   // RepeatInteger    -> value
   // RepeatEnumerated -> index | value return value at index if cast-able to integer, otherwise return index ******
   // RepeatDay        -> value
   virtual long value() const = 0;

   // Depending on the kind of repeat the returned can be value or *current* index
   // RepeatDate       -> value
   // RepeatString     -> index  ( will always return a index)
   // RepeatInteger    -> value
   // RepeatEnumerated -> index  ( will always return a index)
   // RepeatDay        -> value
   virtual long index_or_value() const = 0;
   virtual void increment() = 0;

   // returns a value with in the range start/end
   // Hence at Repeat expiration will return value associated with end()
   virtual long last_valid_value() const  = 0;
   virtual long last_valid_value_minus(int val) const { return last_valid_value() - val;}
   virtual long last_valid_value_plus(int val)  const { return last_valid_value() + val;}

   virtual RepeatBase* clone() const = 0;
   virtual bool compare(RepeatBase*) const = 0;
   virtual bool valid() const = 0;
   virtual void setToLastValue()  = 0;
   virtual std::string valueAsString() const = 0;            // uses last_valid_value
   virtual std::string value_as_string(int index) const = 0;// used in test only
   virtual void reset()  = 0;
   virtual void change(const std::string& newValue) = 0; // can throw std::runtime_error
   virtual void changeValue(long newValue) = 0;          // can throw std::runtime_error
   virtual void set_value(long new_value_or_index) = 0;  // will NOT throw, allows any value
   virtual std::string toString() const = 0;
   virtual std::string dump() const = 0;

   unsigned int state_change_no() const { return state_change_no_;}

   /// Simulator functions:
   virtual bool isInfinite() const  = 0;

   virtual bool is_repeat_day() const { return false; }

protected:
   void incr_state_change_no();

   unsigned int state_change_no_;  // *not* persisted, only used on server side
   std::string name_;
   mutable Variable var_;          // *not* persisted

private:
   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive& ar, const unsigned int /*version*/)
   {
      ar & name_;
   }
};

///
/// The date has no meaning in a physical sense, only used as a for loop over dates
class RepeatDate : public RepeatBase {
public:
   RepeatDate( const std::string& variable, int start, int end, int delta = 1/* always in days*/);
   RepeatDate() :  start_(0), end_(0), delta_(0), value_(0)  {}

   virtual int start() const  { return start_; }
   virtual int end() const    { return end_;   }
   virtual int step() const   { return delta_; }
   virtual long value() const { return value_; }
   virtual long index_or_value() const { return value_;}
   virtual long last_valid_value() const;
   virtual long last_valid_value_minus(int value) const;
   virtual long last_valid_value_plus(int value) const;

   void delta(int d) { delta_ = d;}
   bool operator==(const RepeatDate& rhs) const;

   virtual RepeatDate* clone() const { return new RepeatDate(name_, start_, end_, delta_, value_) ; }
   virtual bool compare(RepeatBase*) const;
   virtual bool valid() const { return (delta_ > 0) ? ( value_ <= end_) : (value_ >= end_); }
   virtual std::string valueAsString() const;
   virtual std::string value_as_string(int index) const;
   virtual void setToLastValue();
   virtual void reset();
   virtual void increment();
   virtual void change(const std::string& newValue); // can throw std::runtime_error
   virtual void changeValue(long newValue);          // can throw std::runtime_error
   virtual void set_value(long newValue);            // will NOT throw, allows any value

   virtual std::string toString() const;
   virtual std::string dump() const;

   /// Simulator functions:
   virtual bool isInfinite() const { return false;}

private:
   RepeatDate( const std::string& name, int start, int end, int delta, long value)
   : RepeatBase(name),start_(start),end_(end),delta_(delta),value_(value) {}

private:
   int  start_;
   int  end_;
   int  delta_;
   long value_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<RepeatBase>(*this);
      ar & start_;
      ar & end_;
      ar & delta_;
      ar & value_;
   }
};

class RepeatInteger : public RepeatBase {
public:
   RepeatInteger( const std::string& variable, int start, int end, int delta = 1);
   RepeatInteger();

   bool operator==(const RepeatInteger& rhs) const;

   virtual int start() const { return start_; }
   virtual int end() const   { return end_; }
   virtual int step() const  { return delta_;}
   virtual long value() const { return value_;}
   virtual long index_or_value() const { return value_;}
   virtual long last_valid_value() const;

   virtual RepeatInteger* clone() const { return new RepeatInteger(name_, start_, end_, delta_, value_); }
   virtual bool compare(RepeatBase*) const;
   virtual bool valid() const { return (delta_ > 0) ? ( value_ <= end_) : (value_ >= end_); }
   virtual std::string valueAsString() const;
   virtual std::string value_as_string(int index) const;
   virtual void setToLastValue();
   virtual void reset();
   virtual void increment();
   virtual void change(const std::string& newValue); // can throw std::runtime_error
   virtual void changeValue(long newValue);          // can throw std::runtime_error
   virtual void set_value(long newValue);            // will NOT throw, allows any value
   virtual std::string toString() const;
   virtual std::string dump() const;

   /// Simulator functions:
   virtual bool isInfinite() const { return false;}

private:
   RepeatInteger( const std::string& name, int start, int end, int delta, long value)
   : RepeatBase(name), start_(start), end_(end), delta_(delta), value_(value) {}

private:
   int  start_;
   int  end_;
   int  delta_;
   long  value_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<RepeatBase>(*this);
      ar & start_;
      ar & end_;
      ar & delta_;
      ar & value_;
   }
};

// Note:: Difference between RepeatEnumerated and  RepeatString, is that
// RepeatEnumerated::value() will return the value at the index if cast-able to integer,
// whereas RepeatString::value() will always return the index.
class RepeatEnumerated : public RepeatBase {
public:
   RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums);
   RepeatEnumerated() : currentIndex_(0) {}

   bool operator==(const RepeatEnumerated& rhs) const;

   virtual int start() const { return 0; }
   virtual int end() const;
   virtual int step() const { return 1 ;}
   virtual long value() const; // return value at index if cast-able to integer, otherwise return index
   virtual long index_or_value() const { return currentIndex_;}
   virtual long last_valid_value() const;

   virtual RepeatBase* clone() const { return new RepeatEnumerated(name_,theEnums_,currentIndex_); }
   virtual bool compare(RepeatBase*) const;
   virtual bool valid() const { return  (currentIndex_ >=0 && currentIndex_ < static_cast<int>(theEnums_.size())); }
   virtual std::string valueAsString() const;
   virtual std::string value_as_string(int index) const;
   virtual void setToLastValue();
   virtual void reset();
   virtual void increment();
   virtual void change(const std::string& newValue); // can throw std::runtime_error
   virtual void changeValue(long newValue);          // can throw std::runtime_error
   virtual void set_value(long newValue);            // will NOT throw, allows any value
   virtual std::string toString() const;
   virtual std::string dump() const;

   /// Simulator functions:
   virtual bool isInfinite() const { return false;}

private:
   RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums, int index)
   : RepeatBase(variable), theEnums_(theEnums), currentIndex_(index) {}

private:
   std::vector<std::string> theEnums_;
   int currentIndex_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<RepeatBase>(*this);
      ar & theEnums_;
      ar & currentIndex_;
   }
};

class RepeatString : public RepeatBase {
public:
   RepeatString( const std::string& variable, const std::vector<std::string>& theEnums);
   RepeatString() : currentIndex_(0) {}

   bool operator==(const RepeatString& rhs) const;

   virtual int start() const { return 0; }
   virtual int end() const;
   virtual int step() const { return 1;}
   virtual long value() const { return currentIndex_;}
   virtual long index_or_value() const { return currentIndex_;}
   virtual long last_valid_value() const; // returns the index

   virtual RepeatBase* clone() const { return new RepeatString(name_,theStrings_,currentIndex_); }
   virtual bool compare(RepeatBase*) const;
   virtual bool valid() const { return  (currentIndex_ >=0 && currentIndex_ < static_cast<int>(theStrings_.size())); }
   virtual std::string valueAsString() const;
   virtual std::string value_as_string(int index) const;
   virtual void setToLastValue();
   virtual void reset();
   virtual void increment();
   virtual void change(const std::string& newValue); // can throw std::runtime_error
   virtual void changeValue(long newValue);          // can throw std::runtime_error
   virtual void set_value(long newValue);            // will NOT throw, allows any value
   virtual std::string toString() const;
   virtual std::string dump() const;

   /// Simulator functions:
   virtual bool isInfinite() const { return false;}

private:
   RepeatString( const std::string& variable, const std::vector<std::string>& theEnums, int index)
   : RepeatBase(variable), theStrings_(theEnums), currentIndex_(index) {}

private:
   std::vector<std::string> theStrings_;
   int currentIndex_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<RepeatBase>(*this);
      ar & theStrings_;
      ar & currentIndex_;
   }
};

/// The current repeat day is not that well defined or deterministic.
///  **** Currently I have not come across any suites that use an end-date ****
///   o If the suite has defined a real clock
///     then number of repeats is deterministic
///     However if the end-date is less than suite clock this should be reported as error
///   o Currently under the hybrid clock the date is not updated, this raises
///     a whole lot of issues. (We don't wont a separate calendar, just for this).
///   o If there is _no_ suite clock, then we must use the current day
///     now the number of repeats  varies, and if end-date is less than the current
///     day we need to report an error
///  Its not clear what behaviour is required here, hence I will not implement
///  the end-date functionality. Until there is clear requirement is this area.
///  end-date will be treated as a parser error.
///  The minimum deterministic functionality here is to implement the infinite
///  repeat that has no end date
///
/// RepeatDay do not really have a name: However we need maintain invariant that all NON-empty repeats
/// have a name. Hence the name will be as day
/// Note: this applies to the clone as well
class RepeatDay : public RepeatBase {
public:
   RepeatDay( int step ) : RepeatBase("day"), step_(step),valid_(true)  {}
   RepeatDay() : step_(1),valid_(true)  {}

   bool operator==(const RepeatDay& rhs) const;

   virtual int start() const { return 0; }
   virtual int end() const   { return 0; }
   virtual int step() const  { return step_;}
   virtual void increment() { /* do nothing */ }
   virtual long value() const { return step_;}
   virtual long index_or_value() const { return step_;}
   virtual long last_valid_value() const { return step_;}

   virtual RepeatBase* clone() const { return new RepeatDay(step_,valid_); }
   virtual bool compare(RepeatBase*) const;
   virtual bool valid() const { return valid_;}
   virtual std::string valueAsString() const { return std::string(); } ;
   virtual std::string value_as_string(int index) const { return std::string(); }
   virtual void setToLastValue() { /* do nothing  ?? */ }
   virtual void reset() {  valid_ = true;  }
   virtual void change(const std::string& /*newValue*/) { /* do nothing */ }
   virtual void changeValue( long /*newValue*/)         { /* do nothing */ }
   virtual void set_value(long /*newValue*/)            { /* do nothing */ }
   virtual std::string toString() const;
   virtual std::string dump() const;

   /// Simulator functions:
   virtual bool isInfinite() const { return true;}

   virtual bool is_repeat_day() const { return true; }

private:
   RepeatDay( int step, bool valid) : RepeatBase("day"), step_(step),valid_(valid)  {}

private:
   int step_;
   bool valid_;  // not persisted since only used in simulator

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & boost::serialization::base_object<RepeatBase>(*this);
      ar & step_;
   }
};

class Repeat {
public:
   Repeat(); // for serialisation
   Repeat( const RepeatDate& );
   Repeat( const RepeatInteger& );
   Repeat( const RepeatEnumerated& );
   Repeat( const RepeatString& );
   Repeat( const RepeatDay& );
   Repeat( const Repeat& );
   ~Repeat();
   Repeat& operator=(const Repeat& rhs);
   bool operator==(const Repeat& rhs) const;

   bool empty() const { return (repeatType_) ? false : true; }
   void clear() { delete repeatType_; repeatType_ = 0;}

   const std::string& name() const;
   const Variable& gen_variable() const;
   void update_repeat_genvar() const;

   int start() const  { return (repeatType_) ? repeatType_->start() : 0;}
   int end() const    { return (repeatType_) ? repeatType_->end()   : 0;}
   int step() const   { return (repeatType_) ? repeatType_->step()  : 0;}
   long value() const { return (repeatType_) ? repeatType_->value() : 0;}
   long index_or_value() const { return (repeatType_) ? repeatType_->index_or_value() : 0;}
   long last_valid_value() const { return (repeatType_) ? repeatType_->last_valid_value() : 0;}
   long last_valid_value_minus(int val) const { return (repeatType_) ? repeatType_->last_valid_value_minus(val) : -val;}
   long last_valid_value_plus(int val)  const { return (repeatType_) ? repeatType_->last_valid_value_plus(val)  : val;}

   std::ostream& print(std::ostream& os) const;
   bool valid() const                           { return (repeatType_) ? repeatType_->valid() : false;}
   void setToLastValue()                        { if (repeatType_) repeatType_->setToLastValue() ; }
   std::string valueAsString() const            { return (repeatType_) ? repeatType_->valueAsString() : std::string(); }
   std::string value_as_string(int index) const { return (repeatType_) ? repeatType_->value_as_string(index) : std::string(); }
   void reset()                                 { if (repeatType_) repeatType_->reset();}
   void increment()                             { if (repeatType_) repeatType_->increment();}
   void change( const std::string& newValue )   { if (repeatType_) repeatType_->change(newValue); }
   void changeValue( long newValue )            { if (repeatType_) repeatType_->changeValue(newValue); }
   void set_value( long newValue )              { if (repeatType_) repeatType_->set_value(newValue); }
   std::string toString() const                 { return (repeatType_) ? repeatType_->toString() : std::string();}
   std::string dump() const                     { return (repeatType_) ? repeatType_->dump() : std::string();} // additional state
   unsigned int state_change_no() const         { return (repeatType_) ? repeatType_->state_change_no() : 0; }

   /// simulator functions:
   bool isInfinite() const                      { return (repeatType_) ? repeatType_->isInfinite() : false;}

   // Allows Repeat's to be returned by reference
   static const Repeat& EMPTY();

   bool is_repeat_day() const { return (repeatType_) ? repeatType_->is_repeat_day() : false; }

   /// Expose base for the GUI only, use with caution
   RepeatBase* repeatBase() const { return repeatType_;}

private:
   RepeatBase* repeatType_;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & repeatType_;
   }
};

BOOST_CLASS_EXPORT_KEY(RepeatDate)
BOOST_CLASS_EXPORT_KEY(RepeatInteger)
BOOST_CLASS_EXPORT_KEY(RepeatEnumerated)
BOOST_CLASS_EXPORT_KEY(RepeatString)
BOOST_CLASS_EXPORT_KEY(RepeatDay)

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(Repeat, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(Repeat,boost::serialization::track_never);

#endif
