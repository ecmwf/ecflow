// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_attribute_NodeAttr_HPP
#define ecflow_attribute_NodeAttr_HPP

#include <limits> // for std::numeric_limits<int>::max()
#include <string>
#include <vector>

namespace cereal {
class access;
}

////////////////////////////////////////////////////////////////////////////////////////
// Class Label:
// Use compiler , generated destructor, assignment,  copy constructor
class Label {
public:
    Label() = default;

    Label(const std::string& name, const std::string& value, const std::string& new_value = "", bool check_name = true);

    const std::string& name() const { return n_; }
    const std::string& value() const { return v_; }
    const std::string& new_value() const { return new_v_; }
    void set_new_value(const std::string& new_label);
    void reset();
    bool empty() const { return n_.empty(); }

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    friend bool operator==(const Label& lhs, const Label& rhs) {
        return lhs.n_ == rhs.n_ && lhs.v_ == rhs.v_ && lhs.new_v_ == rhs.new_v_;
    }

    friend bool operator!=(const Label& lhs, const Label& rhs) { return !(lhs == rhs); }

    bool operator<(const Label& rhs) const { return n_ < rhs.name(); }

    std::string toString() const;
    std::string dump() const;

    ///
    /// @brief Parses a label line into this label.
    ///
    /// @see Label::parse(const std::string&, std::vector<std::string>&, bool, std::string&, std::string&, std::string&)
    ///
    void parse(const std::string& line, std::vector<std::string>& lineTokens, bool parse_state);

    ///
    /// @brief Parses a label line, as found in a definition file, a checkpoint file, or a synchronisation message.
    ///
    /// The accepted forms are:
    ///
    ///   label <name> <value>                                   (unquoted single token)
    ///   label <name> "<default value>"                         (single or double quotes)
    ///   label <name> "<default value>" # <comment>             (definition files only)
    ///   label <name> "<default value>" # "<current value>"     (state, when parse_state is true)
    ///
    /// The value is delimited by the quote character that opens it, and its content is taken verbatim:
    /// blanks, tabs, and embedded quote or hash characters are preserved. The sequence '\n' is
    /// converted to a newline in both values.
    ///
    /// When parse_state is true, the current value is introduced by the state separator, that is, the
    /// closing quote followed by one or more blanks, '#', one or more blanks, and a double quote; the
    /// first separator found after the opening quote is taken. The current value extends to the last
    /// double quote of the line. When parse_state is false, the default value ends at the first closing
    /// quote that is followed only by blanks or by a comment. When parse_state is true and the line
    /// carries no current value, a trailing comment is tolerated only when it holds no double quote;
    /// otherwise the last double quote of the line is taken as the one closing the default value.
    ///
    /// Since values are stored without escaping, one shape remains ambiguous: a default value that
    /// contains its own quote character followed by blanks and '#'. In state form such a value is split at
    /// the wrong place; in a definition file with a trailing comment, it is truncated at that point.
    /// When a state line holds more than one separator, or when a definition line holds a further quote
    /// after the closing quote, a warning naming the label, the values read and the line is logged, so
    /// that the possible truncation does not pass unnoticed.
    ///
    /// @param[in] line the complete line
    /// @param[in] lineTokens the line split at blanks; the second token is the label name
    /// @param[in] parse_state true when the line may carry the current value after the '#' separator
    /// @param[out] the_name the label name
    /// @param[out] the_value the default value
    /// @param[out] the_new_value the current value, or empty when absent
    /// @throws std::runtime_error when the line has fewer than three tokens
    ///
    static void parse(const std::string& line,
                      std::vector<std::string>& lineTokens,
                      bool parse_state,
                      std::string& the_name,
                      std::string& the_value,
                      std::string& the_new_value);
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
// Do not use -1, to represent that no number was specified, as on
// AIX portable binary archive cannot cope with this
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
// For this class we do not check the value member for the equality functionality
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
