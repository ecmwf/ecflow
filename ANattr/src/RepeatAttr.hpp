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

#include <iosfwd>
#include <memory>
#include <boost/utility.hpp>
#include "Variable.hpp"

/////////////////////////////////////////////////////////////////////////
// Node can only have one repeat.
// 
class RepeatBase {
public:
   explicit RepeatBase(const std::string& name) : state_change_no_(0), name_(name) {}
   RepeatBase() = default;
   virtual ~RepeatBase();

   /// make non virtual so that it can be in-lined. Called millions of times
   const std::string& name() const { return name_; }
   virtual int start() const = 0;
   virtual int end() const = 0;
   virtual int step() const = 0;

   // Handle generated variables
   virtual void gen_variables(std::vector<Variable>& vec) const { vec.push_back(var_);}
   virtual const Variable& find_gen_variable(const std::string& name) const { return name == name_ ? var_ : Variable::EMPTY(); }
   virtual void update_repeat_genvar() const;

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

   unsigned int state_change_no_{0};  // *not* persisted, only used on server side
   std::string name_;
   mutable Variable var_;          // *not* persisted

private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar);
};

///
/// The date has no meaning in a physical sense, only used as a for loop over dates
class RepeatDate : public RepeatBase {
public:
   RepeatDate( const std::string& variable, int start, int end, int delta = 1/* always in days*/);
   RepeatDate() = default;

   void gen_variables(std::vector<Variable>& vec) const override;
   const Variable& find_gen_variable(const std::string& name) const override;
   void update_repeat_genvar() const override;

   int start() const override  { return start_; }
   int end() const override    { return end_;   }
   int step() const override   { return delta_; }
   long value() const override { return value_; }
   long index_or_value() const override { return value_;}
   long last_valid_value() const override;
   long last_valid_value_minus(int value) const override;
   long last_valid_value_plus(int value) const override;

   void delta(int d) { delta_ = d;}
   bool operator==(const RepeatDate& rhs) const;

   RepeatDate* clone() const override { return new RepeatDate(name_, start_, end_, delta_, value_) ; }
   bool compare(RepeatBase*) const override;
   bool valid() const override { return (delta_ > 0) ? ( value_ <= end_) : (value_ >= end_); }
   std::string valueAsString() const override;
   std::string value_as_string(int index) const override;
   void setToLastValue() override;
   void reset() override;
   void increment() override;
   void change(const std::string& newValue) override; // can throw std::runtime_error
   void changeValue(long newValue) override;          // can throw std::runtime_error
   void set_value(long newValue) override;            // will NOT throw, allows any value

   std::string toString() const override;
   std::string dump() const override;

   /// Simulator functions:
   bool isInfinite() const override { return false;}

private:
   RepeatDate( const std::string& name, int start, int end, int delta, long value)
   : RepeatBase(name),start_(start),end_(end),delta_(delta),value_(value) {}

private:
   int  start_{0};
   int  end_{0};
   int  delta_{0};
   long value_{0};

   mutable Variable yyyy_;         // *not* persisted
   mutable Variable mm_;           // *not* persisted
   mutable Variable dom_;          // *not* persisted
   mutable Variable dow_;          // *not* persisted
   mutable Variable julian_;       // *not* persisted

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

class RepeatInteger : public RepeatBase {
public:
   RepeatInteger( const std::string& variable, int start, int end, int delta = 1);
   RepeatInteger();

   bool operator==(const RepeatInteger& rhs) const;

   int start() const override { return start_; }
   int end() const override   { return end_; }
   int step() const override  { return delta_;}
   long value() const override { return value_;}
   long index_or_value() const override { return value_;}
   long last_valid_value() const override;

   RepeatInteger* clone() const override { return new RepeatInteger(name_, start_, end_, delta_, value_); }
   bool compare(RepeatBase*) const override;
   bool valid() const override { return (delta_ > 0) ? ( value_ <= end_) : (value_ >= end_); }
   std::string valueAsString() const override;
   std::string value_as_string(int index) const override;
   void setToLastValue() override;
   void reset() override;
   void increment() override;
   void change(const std::string& newValue) override; // can throw std::runtime_error
   void changeValue(long newValue) override;          // can throw std::runtime_error
   void set_value(long newValue) override;            // will NOT throw, allows any value
   std::string toString() const override;
   std::string dump() const override;

   /// Simulator functions:
   bool isInfinite() const override { return false;}

private:
   RepeatInteger( const std::string& name, int start, int end, int delta, long value)
   : RepeatBase(name), start_(start), end_(end), delta_(delta), value_(value) {}

private:
   int  start_{ 0 };
   int  end_{ 0 };
   int  delta_{ 0 };
   long  value_{ 0 };

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

// Note:: Difference between RepeatEnumerated and  RepeatString, is that
// RepeatEnumerated::value() will return the value at the index if cast-able to integer,
// whereas RepeatString::value() will always return the index.
class RepeatEnumerated : public RepeatBase {
public:
   RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums);
   RepeatEnumerated() = default;

   bool operator==(const RepeatEnumerated& rhs) const;

   int start() const override { return 0; }
   int end() const override;
   int step() const override { return 1 ;}
   long value() const override; // return value at index if cast-able to integer, otherwise return index
   long index_or_value() const override { return currentIndex_;}
   long last_valid_value() const override;

   RepeatBase* clone() const override { return new RepeatEnumerated(name_,theEnums_,currentIndex_); }
   bool compare(RepeatBase*) const override;
   bool valid() const override { return  (currentIndex_ >=0 && currentIndex_ < static_cast<int>(theEnums_.size())); }
   std::string valueAsString() const override;
   std::string value_as_string(int index) const override;
   void setToLastValue() override;
   void reset() override;
   void increment() override;
   void change(const std::string& newValue) override; // can throw std::runtime_error
   void changeValue(long newValue) override;          // can throw std::runtime_error
   void set_value(long newValue) override;            // will NOT throw, allows any value
   std::string toString() const override;
   std::string dump() const override;

   /// Simulator functions:
   bool isInfinite() const override { return false;}

private:
   RepeatEnumerated( const std::string& variable, const std::vector<std::string>& theEnums, int index)
   : RepeatBase(variable), theEnums_(theEnums), currentIndex_(index) {}

private:
   std::vector<std::string> theEnums_;
   int currentIndex_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

class RepeatString : public RepeatBase {
public:
   RepeatString( const std::string& variable, const std::vector<std::string>& theEnums);
   RepeatString() = default;

   bool operator==(const RepeatString& rhs) const;

   int start() const override { return 0; }
   int end() const override;
   int step() const override { return 1;}
   long value() const override { return currentIndex_;}
   long index_or_value() const override { return currentIndex_;}
   long last_valid_value() const override; // returns the index

   RepeatBase* clone() const override { return new RepeatString(name_,theStrings_,currentIndex_); }
   bool compare(RepeatBase*) const override;
   bool valid() const override { return  (currentIndex_ >=0 && currentIndex_ < static_cast<int>(theStrings_.size())); }
   std::string valueAsString() const override;
   std::string value_as_string(int index) const override;
   void setToLastValue() override;
   void reset() override;
   void increment() override;
   void change(const std::string& newValue) override; // can throw std::runtime_error
   void changeValue(long newValue) override;          // can throw std::runtime_error
   void set_value(long newValue) override;            // will NOT throw, allows any value
   std::string toString() const override;
   std::string dump() const override;

   /// Simulator functions:
   bool isInfinite() const override { return false;}

private:
   RepeatString( const std::string& variable, const std::vector<std::string>& theEnums, int index)
   : RepeatBase(variable), theStrings_(theEnums), currentIndex_(index) {}

private:
   std::vector<std::string> theStrings_;
   int currentIndex_{0};

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
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
   RepeatDay() = default;

   bool operator==(const RepeatDay& rhs) const;

   int start() const override { return 0; }
   int end() const override   { return 0; }
   int step() const override  { return step_;}
   void increment() override { /* do nothing */ }
   long value() const override { return step_;}
   long index_or_value() const override { return step_;}
   long last_valid_value() const override { return step_;}

   RepeatBase* clone() const override { return new RepeatDay(step_,valid_); }
   bool compare(RepeatBase*) const override;
   bool valid() const override { return valid_;}
   std::string valueAsString() const override { return std::string(); } ;
   std::string value_as_string(int ) const override { return std::string(); }
   void setToLastValue() override { /* do nothing  ?? */ }
   void reset() override {  valid_ = true;  }
   void change(const std::string& /*newValue*/) override { /* do nothing */ }
   void changeValue( long /*newValue*/) override         { /* do nothing */ }
   void set_value(long /*newValue*/) override            { /* do nothing */ }
   std::string toString() const override;
   std::string dump() const override;

   /// Simulator functions:
   bool isInfinite() const override { return true;}

   bool is_repeat_day() const override { return true; }

private:
   RepeatDay( int step, bool valid) : RepeatBase("day"), step_(step),valid_(valid)  {}

private:
   int step_{1};
   bool valid_{true};  // not persisted since only used in simulator

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
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

   bool empty() const { return (type_) ? false : true; }
   void clear() { type_.reset(nullptr); }

   const std::string& name() const;

   void gen_variables(std::vector<Variable>& vec) const { if (type_) type_->gen_variables(vec);}
   const Variable& find_gen_variable(const std::string& name) const { return (type_) ? type_->find_gen_variable(name) : Variable::EMPTY();}
   void update_repeat_genvar() const { if (type_) type_->update_repeat_genvar();}

   int start() const  { return (type_) ? type_->start() : 0;}
   int end() const    { return (type_) ? type_->end()   : 0;}
   int step() const   { return (type_) ? type_->step()  : 0;}
   long value() const { return (type_) ? type_->value() : 0;}
   long index_or_value() const { return (type_) ? type_->index_or_value() : 0;}
   long last_valid_value() const { return (type_) ? type_->last_valid_value() : 0;}
   long last_valid_value_minus(int val) const { return (type_) ? type_->last_valid_value_minus(val) : -val;}
   long last_valid_value_plus(int val)  const { return (type_) ? type_->last_valid_value_plus(val)  : val;}

   std::ostream& print(std::ostream& os) const;
   bool valid() const                           { return (type_) ? type_->valid() : false;}
   void setToLastValue()                        { if (type_) type_->setToLastValue() ; }
   std::string valueAsString() const            { return (type_) ? type_->valueAsString() : std::string(); }
   std::string value_as_string(int index) const { return (type_) ? type_->value_as_string(index) : std::string(); }
   void reset()                                 { if (type_) type_->reset();}
   void increment()                             { if (type_) type_->increment();}
   void change( const std::string& newValue )   { if (type_) type_->change(newValue); }
   void changeValue( long newValue )            { if (type_) type_->changeValue(newValue); }
   void set_value( long newValue )              { if (type_) type_->set_value(newValue); }
   std::string toString() const                 { return (type_) ? type_->toString() : std::string();}
   std::string dump() const                     { return (type_) ? type_->dump() : std::string();} // additional state
   unsigned int state_change_no() const         { return (type_) ? type_->state_change_no() : 0; }

   /// simulator functions:
   bool isInfinite() const                      { return (type_) ? type_->isInfinite() : false;}

   // Allows Repeat's to be returned by reference
   static const Repeat& EMPTY();

   bool is_repeat_day() const { return (type_) ? type_->is_repeat_day() : false; }

   /// Expose base for the GUI only, use with caution
   RepeatBase* repeatBase() const { return type_.get();}

private:
   std::unique_ptr<RepeatBase> type_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

#endif
