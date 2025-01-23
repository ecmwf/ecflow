/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_RepeatAttr_HPP
#define ecflow_attribute_RepeatAttr_HPP

///
/// \brief Repeat Attribute. Please note that for repeat string, enumeration
///        the positional index is used for evaluation.
///
/// Simulation: Simulation must not affect the real job submission in the server.
///   o Infinite repeats cause problems with simulation, hence we have
///     a mechanism to stop this, when reset is called, via server this is disabled
///

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Cal.hpp"
#include "ecflow/core/Chrono.hpp"

/////////////////////////////////////////////////////////////////////////
// Node can only have one repeat.
//
class RepeatBase {
public:
    explicit RepeatBase(const std::string& name) : name_(name) {}
    RepeatBase() = default;
    virtual ~RepeatBase();

    /// make non virtual so that it can be in-lined. Called millions of times
    const std::string& name() const { return name_; }
    virtual int start() const = 0;
    virtual int end() const   = 0;
    virtual int step() const  = 0;

    // Handle generated variables
    virtual void gen_variables(std::vector<Variable>& vec) const { vec.push_back(var_); }
    virtual const Variable& find_gen_variable(const std::string& name) const {
        return name == name_ ? var_ : Variable::EMPTY();
    }
    virtual void update_repeat_genvar() const;

    // After Repeat expiration the last call to increment() can cause
    // value to be beyond the last valid value
    // Depending on the kind of repeat the returned can be value or the current index
    // RepeatDate       -> value
    // RepeatDateTime   -> value
    // RepeatDateList   -> value
    // RepeatString     -> index into array of strings
    // RepeatInteger    -> value
    // RepeatEnumerated -> index | value return value at index if cast-able to integer, otherwise return index ******
    // RepeatDay        -> value
    virtual long value() const = 0;

    // Depending on the kind of repeat the returned can be value or *current* index
    // RepeatDate       -> value
    // RepeatDateTime   -> value
    // RepeatDateList   -> value
    // RepeatString     -> index  ( will always return a index)
    // RepeatInteger    -> value
    // RepeatEnumerated -> index  ( will always return a index)
    // RepeatDay        -> value
    virtual long index_or_value() const = 0;
    virtual void increment()            = 0;

    // returns a value with in the range start/end
    // Hence at Repeat expiration will return value associated with end()
    virtual long last_valid_value() const = 0;
    virtual long last_valid_value_minus(int val) const { return last_valid_value() - val; }
    virtual long last_valid_value_plus(int val) const { return last_valid_value() + val; }

    virtual RepeatBase* clone() const                    = 0;
    virtual bool compare(RepeatBase*) const              = 0;
    virtual bool valid() const                           = 0;
    virtual void setToLastValue()                        = 0;
    virtual std::string valueAsString() const            = 0; // uses last_valid_value
    virtual std::string value_as_string(int index) const = 0; // used in test only
    virtual std::string next_value_as_string() const     = 0;
    virtual std::string prev_value_as_string() const     = 0;
    virtual void reset()                                 = 0;
    virtual void change(const std::string& newValue)     = 0; // can throw std::runtime_error
    virtual void changeValue(long newValue)              = 0; // can throw std::runtime_error
    virtual void set_value(long new_value_or_index)      = 0; // will NOT throw, allows any value
    std::string toString() const;
    virtual void write(std::string&) const = 0;
    virtual std::string dump() const       = 0;

    unsigned int state_change_no() const { return state_change_no_; }

    /// Simulator functions:
    virtual bool isInfinite() const = 0;

    virtual bool is_repeat_day() const { return false; }

    virtual bool isDate() const { return false; }
    virtual bool isDateTime() const { return false; }
    virtual bool isDateList() const { return false; }
    virtual bool isInteger() const { return false; }
    virtual bool isEnumerated() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool isDay() const { return false; }

protected:
    void incr_state_change_no();

    mutable Variable var_; // *not* persisted
    std::string name_;
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

///
/// The date has no meaning in a physical sense, only used as a for loop over dates
class RepeatDate final : public RepeatBase {
public:
    RepeatDate(const std::string& variable, int start, int end, int delta = 1 /* always in days*/);
    RepeatDate() = default;

    void gen_variables(std::vector<Variable>& vec) const override;
    const Variable& find_gen_variable(const std::string& name) const override;
    void update_repeat_genvar() const override;

    int start() const override { return start_; }
    int end() const override { return end_; }
    int step() const override { return delta_; }
    long value() const override { return value_; }
    long index_or_value() const override { return value_; }
    long last_valid_value() const override;
    long last_valid_value_minus(int value) const override;
    long last_valid_value_plus(int value) const override;

    void delta(int d) { delta_ = d; }
    int delta() const { return delta_; }
    bool operator==(const RepeatDate& rhs) const;
    bool operator<(const RepeatDate& rhs) const { return name() < rhs.name(); }

    RepeatDate* clone() const override { return new RepeatDate(name_, start_, end_, delta_, value_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return (delta_ > 0) ? (value_ <= end_) : (value_ >= end_); }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;

    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value

    void write(std::string&) const override;

    std::string dump() const override;
    bool isDate() const override { return true; }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    long valid_value(long value) const;

    RepeatDate(const std::string& name, int start, int end, int delta, long value)
        : RepeatBase(name),
          start_(start),
          end_(end),
          delta_(delta),
          value_(value) {}

    void update_repeat_genvar_value() const;

private:
    int start_{0};
    int end_{0};
    int delta_{0};
    long value_{0};

    mutable Variable yyyy_;   // *not* persisted
    mutable Variable mm_;     // *not* persisted
    mutable Variable dom_;    // *not* persisted
    mutable Variable dow_;    // *not* persisted
    mutable Variable julian_; // *not* persisted

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

class RepeatDateTime final : public RepeatBase {
public:
    RepeatDateTime(const std::string& variable,
                   const std::string& start,
                   const std::string& end,
                   const std::string& delta = "24:00:00");
    RepeatDateTime(const std::string& variable, ecf::Instant start, ecf::Instant end, ecf::Duration delta);
    RepeatDateTime() = default;

    void gen_variables(std::vector<Variable>& vec) const override;
    const Variable& find_gen_variable(const std::string& name) const override;
    void update_repeat_genvar() const override;

    const ecf::Instant& start_instant() const { return start_; }
    const ecf::Instant& end_instant() const { return end_; }
    const ecf::Duration& step_duration() const { return delta_; }
    const ecf::Instant& value_instant() const { return value_; }

    int start() const override { return coerce_from_instant(start_); }
    int end() const override { return coerce_from_instant(end_); }
    int step() const override { return delta_.as_seconds().count(); }
    long value() const override { return coerce_from_instant(value_); }
    long index_or_value() const override { return coerce_from_instant(value_); }
    long last_valid_value() const override;
    long last_valid_value_minus(int value) const override;
    long last_valid_value_plus(int value) const override;

    void delta(const ecf::Duration& d) { delta_ = d; }
    bool operator==(const RepeatDateTime& rhs) const;
    bool operator<(const RepeatDateTime& rhs) const { return name() < rhs.name(); }

    RepeatDateTime* clone() const override { return new RepeatDateTime(name_, start_, end_, delta_, value_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override {
        return (delta_ > ecf::Duration{std::chrono::seconds{0}}) ? (value_ <= end_) : (value_ >= end_);
    }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;

    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value

    void write(std::string&) const override;

    std::string dump() const override;
    bool isDateTime() const override { return true; }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    ecf::Instant valid_value(const ecf::Instant& value) const;

    RepeatDateTime(const std::string& name,
                   ecf::Instant start,
                   ecf::Instant end,
                   ecf::Duration delta,
                   ecf::Instant value)
        : RepeatBase(name),
          start_(start),
          end_(end),
          delta_(delta),
          value_(value) {}

    void update_repeat_genvar_value() const;

private:
    ecf::Instant start_;
    ecf::Instant end_;
    ecf::Duration delta_;
    ecf::Instant value_;

    // *not* persisted
    mutable VariableMap generated_{
        // clang-format off
        // Date
        Variable(name_ + "_DATE", "<invalid>"),
        // Date Components
        Variable(name_ + "_YYYY", "<invalid>"),
        Variable(name_ + "_MM", "<invalid>"),
        Variable(name_ + "_DD", "<invalid>"),
        Variable(name_ + "_JULIAN", "<invalid>"),
        // Time
        Variable(name_ + "_TIME", "<invalid>"),
        // Time Components4
        Variable(name_ + "_HOURS", "<invalid>"),
        Variable(name_ + "_MINUTES", "<invalid>"),
        Variable(name_ + "_SECONDS", "<invalid>")
        // clang-format on
    };

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

class RepeatDateList final : public RepeatBase {
public:
    RepeatDateList(const std::string& variable, const std::vector<int>&); // will throw for empty list
    RepeatDateList() = default;

    void gen_variables(std::vector<Variable>& vec) const override;
    const Variable& find_gen_variable(const std::string& name) const override;
    void update_repeat_genvar() const override;

    bool operator==(const RepeatDateList& rhs) const;
    bool operator<(const RepeatDateList& rhs) const { return name() < rhs.name(); }

    int start() const override;
    int end() const override;
    int step() const override { return 1; }
    long value() const override; // return value at index   otherwise return 0
    long value_at(size_t i) const {
        assert(i < list_.size());
        return list_[i];
    };
    long index_or_value() const override { return currentIndex_; }
    long last_valid_value() const override;
    long last_valid_value_minus(int value) const override;
    long last_valid_value_plus(int value) const override;

    RepeatBase* clone() const override { return new RepeatDateList(name_, list_, currentIndex_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(list_.size())); }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;

    const std::vector<int>& values() const { return list_; }

    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value
    void write(std::string&) const override;
    std::string dump() const override;
    bool isDateList() const override { return true; }
    int indexNum() const { return static_cast<int>(list_.size()); }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    RepeatDateList(const std::string& variable, const std::vector<int>& l, int index)
        : RepeatBase(variable),
          currentIndex_(index),
          list_(l) {}

    void update_repeat_genvar_value() const;

private:
    int currentIndex_{0};
    std::vector<int> list_;

    mutable Variable yyyy_;   // *not* persisted
    mutable Variable mm_;     // *not* persisted
    mutable Variable dom_;    // *not* persisted
    mutable Variable dow_;    // *not* persisted
    mutable Variable julian_; // *not* persisted

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

class RepeatInteger final : public RepeatBase {
public:
    RepeatInteger(const std::string& variable, int start, int end, int delta = 1);
    RepeatInteger();

    bool operator==(const RepeatInteger& rhs) const;
    bool operator<(const RepeatInteger& rhs) const { return name() < rhs.name(); }

    int start() const override { return start_; }
    int end() const override { return end_; }
    int step() const override { return delta_; }
    long value() const override { return value_; }
    long index_or_value() const override { return value_; }
    long last_valid_value() const override;
    int delta() const { return delta_; }

    RepeatInteger* clone() const override { return new RepeatInteger(name_, start_, end_, delta_, value_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return (delta_ > 0) ? (value_ <= end_) : (value_ >= end_); }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;
    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value
    void write(std::string&) const override;
    std::string dump() const override;
    bool isInteger() const override { return true; }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    long valid_value(long value) const;

    RepeatInteger(const std::string& name, int start, int end, int delta, long value)
        : RepeatBase(name),
          start_(start),
          end_(end),
          delta_(delta),
          value_(value) {}

private:
    int start_{0};
    int end_{0};
    int delta_{0};
    long value_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

// Note:: Difference between RepeatEnumerated and  RepeatString, is that
// RepeatEnumerated::value() will return the value at the index if cast-able to integer,
// whereas RepeatString::value() will always return the index.
class RepeatEnumerated final : public RepeatBase {
public:
    RepeatEnumerated(const std::string& variable, const std::vector<std::string>& theEnums);
    RepeatEnumerated() = default;

    bool operator==(const RepeatEnumerated& rhs) const;
    bool operator<(const RepeatEnumerated& rhs) const { return name() < rhs.name(); }

    int start() const override { return 0; }
    int end() const override;
    int step() const override { return 1; }
    long value() const override; // return value at index if cast-able to integer, otherwise return index
    long index_or_value() const override { return currentIndex_; }
    long last_valid_value() const override;

    const std::vector<std::string>& values() const { return theEnums_; }

    RepeatBase* clone() const override { return new RepeatEnumerated(name_, theEnums_, currentIndex_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theEnums_.size())); }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;

    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value
    void write(std::string&) const override;
    std::string dump() const override;
    bool isEnumerated() const override { return true; }
    int indexNum() const { return static_cast<int>(theEnums_.size()); }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    RepeatEnumerated(const std::string& variable, const std::vector<std::string>& theEnums, int index)
        : RepeatBase(variable),
          currentIndex_(index),
          theEnums_(theEnums) {}

private:
    int currentIndex_{0};
    std::vector<std::string> theEnums_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

class RepeatString final : public RepeatBase {
public:
    RepeatString(const std::string& variable, const std::vector<std::string>& theEnums);
    RepeatString() = default;

    bool operator==(const RepeatString& rhs) const;
    bool operator<(const RepeatString& rhs) const { return name() < rhs.name(); }

    int start() const override { return 0; }
    int end() const override;
    int step() const override { return 1; }
    long value() const override { return currentIndex_; }
    long index_or_value() const override { return currentIndex_; }
    long last_valid_value() const override; // returns the index

    RepeatBase* clone() const override { return new RepeatString(name_, theStrings_, currentIndex_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theStrings_.size())); }
    std::string valueAsString() const override;
    std::string value_as_string(int index) const override;
    std::string next_value_as_string() const override;
    std::string prev_value_as_string() const override;

    const std::vector<std::string>& values() const { return theStrings_; }

    void setToLastValue() override;
    void reset() override;
    void increment() override;
    void change(const std::string& newValue) override; // can throw std::runtime_error
    void changeValue(long newValue) override;          // can throw std::runtime_error
    void set_value(long newValue) override;            // will NOT throw, allows any value
    void write(std::string&) const override;
    std::string dump() const override;
    bool isString() const override { return true; }
    int indexNum() const { return static_cast<int>(theStrings_.size()); }

    /// Simulator functions:
    bool isInfinite() const override { return false; }

private:
    RepeatString(const std::string& variable, const std::vector<std::string>& theEnums, int index)
        : RepeatBase(variable),
          currentIndex_(index),
          theStrings_(theEnums) {}

private:
    int currentIndex_{0};
    std::vector<std::string> theStrings_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
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
class RepeatDay final : public RepeatBase {
public:
    RepeatDay() : RepeatBase("day") {}

    // Enable implicit conversion from integer
    RepeatDay(int step) : RepeatBase("day"), step_(step) {}

    bool operator==(const RepeatDay& rhs) const;
    bool operator<(const RepeatDay& rhs) const { return step_ < rhs.step(); }

    int start() const override { return 0; }
    int end() const override { return 0; }
    int step() const override { return step_; }
    void increment() override { /* do nothing */ }
    long value() const override { return step_; }
    long index_or_value() const override { return step_; }
    long last_valid_value() const override { return step_; }

    RepeatBase* clone() const override { return new RepeatDay(step_, valid_); }
    bool compare(RepeatBase*) const override;
    bool valid() const override { return valid_; }
    std::string valueAsString() const override { return std::string(); };
    std::string value_as_string(int) const override { return std::string(); }
    std::string next_value_as_string() const override { return std::string(); }
    std::string prev_value_as_string() const override { return std::string(); }

    void setToLastValue() override { /* do nothing  ?? */ }
    void reset() override { valid_ = true; }
    void change(const std::string& /*newValue*/) override { /* do nothing */ }
    void changeValue(long /*newValue*/) override { /* do nothing */ }
    void set_value(long /*newValue*/) override { /* do nothing */ }
    void write(std::string&) const override;
    std::string dump() const override;
    bool isDay() const override { return true; }

    /// Simulator functions:
    bool isInfinite() const override { return true; }

    bool is_repeat_day() const override { return true; }

private:
    RepeatDay(int step, bool valid) : RepeatBase("day"), step_(step), valid_(valid) {}

private:
    int step_{1};
    bool valid_{true}; // not persisted since only used in simulator

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

class Repeat {
public:
    Repeat(); // for serialisation

    // Enable implicit conversion to Repeat
    Repeat(const RepeatDate&);
    Repeat(const RepeatDateTime&);
    Repeat(const RepeatDateList&);
    Repeat(const RepeatInteger&);
    Repeat(const RepeatEnumerated&);
    Repeat(const RepeatString&);
    Repeat(const RepeatDay&);

    // Enable copy semantics
    Repeat(const Repeat&);
    Repeat& operator=(const Repeat& rhs);

    // Enable move semantics
    Repeat(Repeat&& rhs) : type_(std::move(rhs.type_)) {}
    Repeat& operator=(Repeat&& rhs);

    ~Repeat();

    bool operator==(const Repeat& rhs) const;
    bool operator<(const Repeat& rhs) const { return name() < rhs.name(); }

    bool empty() const { return (type_) ? false : true; }
    void clear() { type_.reset(nullptr); }

    const std::string& name() const;

    void gen_variables(std::vector<Variable>& vec) const {
        if (type_)
            type_->gen_variables(vec);
    }
    const Variable& find_gen_variable(const std::string& name) const {
        return (type_) ? type_->find_gen_variable(name) : Variable::EMPTY();
    }
    void update_repeat_genvar() const {
        if (type_)
            type_->update_repeat_genvar();
    }

    int start() const { return (type_) ? type_->start() : 0; }
    int end() const { return (type_) ? type_->end() : 0; }
    int step() const { return (type_) ? type_->step() : 0; }
    long value() const { return (type_) ? type_->value() : 0; }
    long index_or_value() const { return (type_) ? type_->index_or_value() : 0; }
    long last_valid_value() const { return (type_) ? type_->last_valid_value() : 0; }
    long last_valid_value_minus(int val) const { return (type_) ? type_->last_valid_value_minus(val) : -val; }
    long last_valid_value_plus(int val) const { return (type_) ? type_->last_valid_value_plus(val) : val; }

    void print(std::string& os) const;
    bool valid() const { return (type_) ? type_->valid() : false; }
    void setToLastValue() {
        if (type_)
            type_->setToLastValue();
    }
    std::string valueAsString() const { return (type_) ? type_->valueAsString() : std::string(); }
    std::string value_as_string(int index) const { return (type_) ? type_->value_as_string(index) : std::string(); }
    std::string next_value_as_string() const { return (type_) ? type_->next_value_as_string() : std::string(); }
    std::string prev_value_as_string() const { return (type_) ? type_->prev_value_as_string() : std::string(); }

    void reset() {
        if (type_)
            type_->reset();
    }
    void increment() {
        if (type_)
            type_->increment();
    }
    void change(const std::string& newValue) {
        if (type_)
            type_->change(newValue);
    }
    void changeValue(long newValue) {
        if (type_)
            type_->changeValue(newValue);
    }
    void set_value(long newValue) {
        if (type_)
            type_->set_value(newValue);
    }
    std::string toString() const { return (type_) ? type_->toString() : std::string(); }
    std::string dump() const { return (type_) ? type_->dump() : std::string(); } // additional state
    unsigned int state_change_no() const { return (type_) ? type_->state_change_no() : 0; }

    /// simulator functions:
    bool isInfinite() const { return (type_) ? type_->isInfinite() : false; }

    // Allows Repeat's to be returned by reference
    static const Repeat& EMPTY();

    bool is_repeat_day() const { return (type_) ? type_->is_repeat_day() : false; }

    /// Expose base for the GUI only, use with caution
    RepeatBase* repeatBase() const { return type_.get(); }

    template <typename T>
    const T& as() const {
        return dynamic_cast<const T&>(*repeatBase());
    }

private:
    void write(std::string& ret) const {
        if (type_)
            type_->write(ret);
    }

private:
    std::unique_ptr<RepeatBase> type_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

#endif /* ecflow_attribute_RepeatAttr_HPP */
