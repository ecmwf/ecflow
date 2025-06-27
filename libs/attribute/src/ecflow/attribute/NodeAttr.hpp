/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_NodeAttr_HPP
#define ecflow_attribute_NodeAttr_HPP

#include <limits> // for std::numeric_limits<int>::max()
#include <string>
#include <vector>

#include <boost/operators.hpp>

namespace cereal {
class access;
}

////////////////////////////////////////////////////////////////////////////////////////
// Class Label:
// Use compiler , generated destructor, assignment,  copy constructor
class Label : public boost::equality_comparable<Label> {
public:
    Label(const std::string& name, const std::string& value, const std::string& new_value = "", bool check_name = true);
    Label() = default;

    const std::string& name() const { return n_; }
    const std::string& value() const { return v_; }
    const std::string& new_value() const { return new_v_; }
    void set_new_value(const std::string& new_label);
    void reset();
    bool empty() const { return n_.empty(); }

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    // 2 kinds of equality, structure and state
    friend bool operator==(const Label& lhs, const Label& rhs) {
        if (lhs.n_ != rhs.n_) {
            // std::cout << "lhs.n_ '" << lhs.n_ << "' != rhs.n_ '" << rhs.n_ << "'\n";
            return false;
        }
        if (lhs.new_v_ != rhs.new_v_) {
            // std::cout << "lhs.new_v_ '" << lhs.new_v_ << "' != rhs.new_v_ '" << rhs.new_v_ << "'\n";
            return false;
        }
        if (lhs.v_ != rhs.v_) {
            // std::cout << "lhs.v_ '" << lhs.v_ << "' != rhs.v_ '" << rhs.v_ << "'\n";
            return false;
        }
        return true;
    }
    bool operator<(const Label& rhs) const { return n_ < rhs.name(); }

    std::string toString() const;
    std::string dump() const;

    void parse(const std::string& line, std::vector<std::string>& lineTokens, bool parse_state);
    static void parse(const std::string& line,
                      std::vector<std::string>& lineTokens,
                      bool parse_state,
                      std::string&,
                      std::string&,
                      std::string&);
    static const Label& EMPTY(); // Added to support return by reference

public:
    void write(std::string&) const;

private:
    std::string n_;
    std::string v_;
    std::string new_v_;
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
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
    explicit Event(int number, const std::string& eventName = "", bool initial_val = false, bool check_name = true);
    explicit Event(const std::string& eventName, bool initial_val = false);
    Event() = default;

    static Event make_from_value(const std::string& name, const std::string& value);

    std::string name_or_number() const; // if name present return, else return number
    const std::string& name() const { return n_; }
    bool value() const { return v_; }
    void reset() { set_value(iv_); }
    bool empty() const { return (n_.empty() && number_ == std::numeric_limits<int>::max()); }
    void set_initial_value(bool iv) { iv_ = iv; }
    bool initial_value() const { return iv_; }

    int number() const { return number_; }
    bool operator==(const Event& rhs) const;
    bool operator<(const Event& rhs) const;
    bool compare(const Event& rhs) const; // two events the same if name/number the same, ignores state
    void set_value(bool b);               // updates state_change_no_
    bool usedInTrigger() const { return used_; }
    void usedInTrigger(bool b) { used_ = b; }

    unsigned int state_change_no() const { return state_change_no_; }

    std::string toString() const;
    std::string dump() const;

    static bool isValidState(const std::string&); // return true for "set" | "clear"
    static const std::string& SET();
    static const std::string& CLEAR();
    static const Event& EMPTY(); // Added to support return by reference

public:
    void write(std::string&) const;

private:
    std::string n_;
    int number_{std::numeric_limits<int>::max()};
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side
    bool v_{false};
    bool iv_{false};   // initial value ECFLOW-1526
    bool used_{false}; // used by the simulator not persisted

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

// Class Meter:
// Use compiler , generated destructor, assignment, copy constructor
// For this class we don't check the value member for the equality functionality
// Can have negative min/max however max >= min, and color change should be in the
// range min-max
class Meter {
public:
    Meter(const std::string& name,
          int min,
          int max,
          int colorChange = std::numeric_limits<int>::max(),
          int value       = std::numeric_limits<int>::max(),
          bool check      = true);
    Meter() = default;

    static Meter make_from_value(const std::string& name, const std::string& value);

    void reset() { set_value(min_); }
    void set_value(int v); // can throw std::runtime_error if out of range
    bool empty() const { return n_.empty(); }

    const std::string& name() const { return n_; }
    int value() const { return v_; }
    int min() const { return min_; }
    int max() const { return max_; }
    int colorChange() const { return cc_; }

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    bool operator==(const Meter& rhs) const;
    bool operator<(const Meter& rhs) const { return n_ < rhs.name(); }

    bool usedInTrigger() const { return used_; }
    void usedInTrigger(bool b) { used_ = b; }
    std::string toString() const;
    std::string dump() const;

    static const Meter& EMPTY(); // Added to support return by reference

public:
    void write(std::string&) const;
private:
    bool isValidValue(int v) const { return (v >= min_ && v <= max_); }

    int min_{0};
    int max_{0};
    int v_{0};                        // value
    int cc_{0};                       // Colour change, used by gui ?
    std::string n_;                   // name
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side
    bool used_{false};                // used by the simulator not persisted

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

#endif /* ecflow_attribute_NodeAttr_HPP */
