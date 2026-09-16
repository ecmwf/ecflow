/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/attribute/NodeAttr.hpp"

#include <sstream>
#include <stdexcept>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace ecf;

const std::string& Event::SET() {
    static const std::string SET = "set";
    return SET;
}
const std::string& Event::CLEAR() {
    static const std::string CLEAR = "clear";
    return CLEAR;
}
const Event& Event::EMPTY() {
    static const Event EVENT = Event();
    return EVENT;
}
const Meter& Meter::EMPTY() {
    static const Meter METER = Meter();
    return METER;
}
const Label& Label::EMPTY() {
    static const Label LABEL = Label();
    return LABEL;
}

////////////////////////////////////////////////////////////////////////////////////////////

Event::Event(int number, const std::string& eventName, bool iv, bool check_name)
    : n_(eventName),
      number_(number),
      v_(iv),
      iv_(iv) {
    if (!eventName.empty() && check_name) {
        std::string msg;
        if (!ecf::algorithm::is_valid_name(eventName, msg)) {
            throw std::runtime_error("Event::Event: Invalid event name : " + msg);
        }
    }
}

Event::Event(const std::string& eventName, bool iv)
    : n_(eventName),
      v_(iv),
      iv_(iv) {
    if (eventName.empty()) {
        throw std::runtime_error("Event::Event: Invalid event name : name must be specified if no number supplied");
    }

    // If the eventName is an integer, then treat it as such, by setting number_ and clearing n_
    // This was added after migration failed, since *python* api allowed:
    //       ta.add_event(1);
    //       ta.add_event("1");
    // and when we called ecflow_client --migrate/--get it generated
    //       event 1
    //       event 1
    // which then did *not* load.
    //
    // Test for numeric, and then casting, is ****faster***** than relying on exception alone
    if (eventName.find_first_of(ecf::string_constants::numeric_chars) == 0) {
        try {
            number_ = ecf::convert_to<int>(eventName);
            n_.clear();
            return;
        }
        catch (const ecf::bad_conversion&) {
            // cast failed, a real string, carry on
        }
    }

    std::string msg;
    if (!ecf::algorithm::is_valid_name(eventName, msg)) {
        throw std::runtime_error("Event::Event: Invalid event name : " + msg);
    }
}

Event Event::make_from_value(const std::string& name, const std::string& value) {

    // value is expected to be either "set" or "clear"

    if (!isValidState(value)) {
        throw std::runtime_error("Event::Event: Invalid state : " + value);
    }
    return Event(name, value == Event::SET());
}

void Event::set_value(bool b) {
    v_               = b;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Event::set_value\n";
#endif
}

std::string Event::name_or_number() const {
    if (n_.empty()) {
        return MESSAGE(number_);
    }
    return n_;
}

bool Event::operator<(const Event& rhs) const {
    if (!n_.empty() && !rhs.name().empty()) {
        return n_ < rhs.name();
    }
    if (n_.empty() && rhs.name().empty()) {
        return number_ < rhs.number();
    }
    return name_or_number() < rhs.name_or_number();
}

bool Event::operator==(const Event& rhs) const {
    if (v_ != rhs.v_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "v_ != rhs.v_   (v_:" << v_ << " rhs.v_:" << rhs.v_ << ") " << toString() << "\n";
        }
#endif
        return false;
    }
    if (number_ != rhs.number_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "number_ != rhs.number_   (number_:" << number_ << " rhs.number_:" << rhs.number_ << ") "
                      << toString() << "\n";
        }
#endif
        return false;
    }
    if (n_ != rhs.n_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "n_ != rhs.n_  (n_:" << n_ << " rhs.n_:" << rhs.n_ << ") " << toString() << "\n";
        }
#endif
        return false;
    }
    if (iv_ != rhs.iv_) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "iv_ != rhs.iv_   (iv_:" << iv_ << " rhs.iv_:" << rhs.iv_ << ") " << toString() << "\n";
        }
#endif
        return false;
    }
    return true;
}

bool Event::compare(const Event& rhs) const {
    if (number_ != rhs.number_) {
        return false;
    }
    if (n_ != rhs.n_) {
        return false;
    }
    return true;
}

std::string Event::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void Event::write(std::string& ret) const {
    ret += "event ";
    if (number_ == std::numeric_limits<int>::max()) {
        ret += n_;
    }
    else {
        ret += ecf::convert_to<std::string>(number_);
        ret += " ";
        ret += n_;
    }

    if (iv_) {
        ret += " set"; // initial value
    }
}

std::string Event::dump() const {
    return MESSAGE(toString() << " value(" << v_ << ")  used(" << used_ << ")");
}

bool Event::isValidState(const std::string& state) {
    return state == Event::SET() || state == Event::CLEAR();
}

////////////////////////////////////////////////////////////////////////////////////////////

Meter::Meter(const std::string& name, int min, int max, int colorChange, int value, bool check)
    : min_(min),
      max_(max),
      v_(value),
      cc_(colorChange),
      n_(name) {
    if (check) {
        if (!ecf::algorithm::is_valid_name(name)) {
            throw std::runtime_error("Meter::Meter: Invalid Meter name: " + name);
        }
    }

    if (min > max) {
        throw std::out_of_range("Meter::Meter: Invalid Meter(name,min,max,color_change) : min must be less than max");
    }

    if (colorChange == std::numeric_limits<int>::max()) {
        cc_ = max_;
    }

    if (value == std::numeric_limits<int>::max()) {
        v_ = min_;
    }

    if (cc_ < min || cc_ > max) {
        throw std::out_of_range(MESSAGE("Meter::Meter: Invalid Meter(name,min,max,color_change) color_change("
                                        << cc_ << ") must be between min(" << min_ << ") and max(" << max_ << ")"));
    }
}

Meter Meter::make_from_value(const std::string& name, const std::string& value) {

    // value is expected to be of the form "min,max,value", where min, max and value are integers

    std::vector<std::string> tokens;
    ecf::algorithm::split_at(tokens, value, ",");
    if (tokens.size() != 3) {
        throw std::runtime_error(
            MESSAGE("Meter::make_from_value: Expect three comma-separated values, but found: '" << value << "'"));
    }

    try {
        auto min   = ecf::convert_to<int>(tokens[0]);
        auto max   = ecf::convert_to<int>(tokens[1]);
        auto value = ecf::convert_to<int>(tokens[2]);
        return Meter(name, min, max, max, value, false);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error(MESSAGE("Meter::make_from_value: Expect three comma-separated values, but found: ("
                                         << tokens[0] << ", " << tokens[1] << ", " << tokens[2] << ")"));
    }
}

void Meter::set_value(int v) {

    if (!isValidValue(v)) {
        throw std::runtime_error(MESSAGE("Meter::set_value(int): The meter(" << n_ << ") value must be in the range["
                                                                             << min() << "->" << max()
                                                                             << "] but found '" << v << "'"));
    }

    v_               = v;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Meter::set_value\n";
#endif
}

bool Meter::operator==(const Meter& rhs) const {
    if (v_ != rhs.v_) {
        return false;
    }
    if (min_ != rhs.min_) {
        return false;
    }
    if (max_ != rhs.max_) {
        return false;
    }
    if (cc_ != rhs.cc_) {
        return false;
    }
    if (n_ != rhs.n_) {
        return false;
    }
    return true;
}

std::string Meter::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void Meter::write(std::string& ret) const {
    ret += "meter ";
    ret += n_;
    ret += " ";
    ret += ecf::convert_to<std::string>(min_);
    ret += " ";
    ret += ecf::convert_to<std::string>(max_);
    ret += " ";
    ret += ecf::convert_to<std::string>(cc_);
}

std::string Meter::dump() const {
    return MESSAGE("meter " << n_ << " min(" << min_ << ") max (" << max_ << ") colorChange(" << cc_ << ") value(" << v_
                            << ") used(" << used_ << ")");
}

/////////////////////////////////////////////////////////////////////////////////////////////

Label::Label(const std::string& name, const std::string& value, const std::string& new_value, bool check_name)
    : n_(name),
      v_(value),
      new_v_(new_value) {
    if (check_name && !ecf::algorithm::is_valid_name(n_)) {
        throw std::runtime_error(MESSAGE("Label::Label: Invalid Label name :" << n_));
    }
}

std::string Label::toString() const {
    // parsing always STRIPS the quotes, hence add them back
    std::string ret;
    ret.reserve(n_.size() + v_.size() + 10);
    write(ret);
    return ret;
}

void Label::write(std::string& ret) const {
    // parsing always STRIPS the quotes, hence add them back
    ret += "label ";
    ret += n_;
    ret += " \"";
    if (v_.find("\n") == std::string::npos) {
        ret += v_;
    }
    else {
        // replace \n, otherwise re-parse will fail
        std::string value = v_;
        ecf::algorithm::replace_all(value, "\n", "\\n");
        ret += value;
    }
    ret += "\"";
}

std::string Label::dump() const {
    return MESSAGE(toString() << " : \"" << new_v_ << "\"");
}

void Label::set_new_value(const std::string& l) {
    new_v_           = l;
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Label::set_new_value\n";
#endif
}

void Label::reset() {
    new_v_.clear();
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Label::reset()\n";
#endif
}

void Label::parse(const std::string& line, std::vector<std::string>& lineTokens, bool parse_state) {
    parse(line, lineTokens, parse_state, n_, v_, new_v_);
}

namespace {

bool is_blank(char c) {
    return c == ' ' || c == '\t';
}

// Skips blanks, then one token, starting at 'pos'; returns the position after the token.
size_t skip_token(const std::string& line, size_t pos) {
    while (pos < line.size() && is_blank(line[pos])) {
        ++pos;
    }
    while (pos < line.size() && !is_blank(line[pos])) {
        ++pos;
    }
    return pos;
}

// Returns true when 'line', from 'pos' to its end, holds only blanks optionally followed by a comment.
// A comment needs at least one blank before '#', so that a quote immediately followed by '#' stays
// part of the value (the documented limitation is a quote followed by one or more blanks and '#').
bool only_blanks_or_comment(const std::string& line, size_t pos) {
    const size_t start = pos;
    while (pos < line.size() && is_blank(line[pos])) {
        ++pos;
    }
    return pos == line.size() || (pos > start && line[pos] == '#');
}

// Searches, from 'from', for the state separator: the quote 'q', one or more blanks, '#',
// one or more blanks, and a double quote. On success, 'closing' receives the position of the
// quote that closes the default value and 'opening' the position of the quote that opens the
// current value.
bool find_state_separator(const std::string& line, size_t from, char q, size_t& closing, size_t& opening) {
    for (size_t pos = line.find(q, from); pos != std::string::npos; pos = line.find(q, pos + 1)) {
        size_t i = pos + 1;
        if (i >= line.size() || !is_blank(line[i])) {
            continue;
        }
        while (i < line.size() && is_blank(line[i])) {
            ++i;
        }
        if (i >= line.size() || line[i] != '#') {
            continue;
        }
        ++i;
        if (i >= line.size() || !is_blank(line[i])) {
            continue;
        }
        while (i < line.size() && is_blank(line[i])) {
            ++i;
        }
        if (i < line.size() && line[i] == '"') {
            closing = pos;
            opening = i;
            return true;
        }
    }
    return false;
}

void unescape_newlines(std::string& value) {
    if (value.find("\\n") != std::string::npos) {
        ecf::algorithm::replace_all(value, "\\n", "\n");
    }
}

// Reads the current value that opens at the double quote found at 'opening'; the value extends to the
// last double quote of the line, or to the end of the line when no other double quote follows.
std::string read_current_value(const std::string& line, size_t opening) {
    size_t end = line.size();
    if (line.back() == '"' && line.size() - 1 > opening) {
        end = line.size() - 1;
    }
    else if (size_t last = line.rfind('"'); last != std::string::npos && last > opening) {
        end = last; // line does not end with the closing quote; ignore what follows the last quote
    }
    std::string value = line.substr(opening + 1, end - opening - 1);
    unescape_newlines(value);
    return value;
}

} // namespace

void Label::parse(const std::string& line,
                  std::vector<std::string>& lineTokens,
                  bool parse_state,
                  std::string& the_name,
                  std::string& the_value,
                  std::string& the_new_value) {
    size_t line_token_size = lineTokens.size();
    if (line_token_size < 3) {
        throw std::runtime_error("Label::parse: Invalid label :" + line);
    }

    the_name = lineTokens[1];
    the_new_value.clear();

    // Locate the first character of the default value: after 'label', the name, and the blanks that follow
    size_t value_begin = skip_token(line, skip_token(line, 0));
    while (value_begin < line.size() && is_blank(line[value_begin])) {
        ++value_begin;
    }

    if (value_begin >= line.size() || (line[value_begin] != '"' && line[value_begin] != '\'')) {
        // A single unquoted token is the whole value, even when it starts with '#', e.g. 'label rev #40fd83'
        if (line_token_size == 3) {
            the_value = lineTokens[2];
            unescape_newlines(the_value);
            return;
        }

        // Unquoted value, e.g. 'label OBS 0'; tokens are joined until a comment starts
        std::string value;
        value.reserve(line.size());
        size_t i = 2;
        for (; i < line_token_size; ++i) {
            if (lineTokens[i].at(0) == '#') {
                break;
            }
            if (i != 2) {
                value += " ";
            }
            value += lineTokens[i];
        }
        the_value = value;
        unescape_newlines(the_value);

        // With state, a '#' token followed by a double quote introduces the current value: label OBS 0 # "current"
        if (parse_state && i + 1 < line_token_size && lineTokens[i] == "#" && lineTokens[i + 1].at(0) == '"') {
            size_t pos = value_begin;
            for (size_t k = 2; k <= i; ++k) {
                pos = skip_token(line, pos); // position right after token k
            }
            size_t opening = line.find('"', pos);
            the_new_value  = read_current_value(line, opening);
        }
        return;
    }

    // Quoted value; the quote character that opens the value is the one that closes it.
    // The content between the quotes is taken verbatim, hence blanks are preserved.
    const char q       = line[value_begin];
    const size_t begin = value_begin + 1;

    size_t closing = std::string::npos;
    bool ambiguous = false;
    if (parse_state) {
        // label name "default value" # "current value"
        size_t opening = std::string::npos;
        if (find_state_separator(line, begin, q, closing, opening)) {
            the_new_value = read_current_value(line, opening);

            // A second separator means that a value contains the separator sequence; the split above may then be wrong
            size_t other_closing = std::string::npos;
            size_t other_opening = std::string::npos;
            ambiguous            = find_state_separator(line, opening, '"', other_closing, other_opening);
        }
    }
    else {
        // label name "default value" # comment
        for (size_t pos = line.find(q, begin); pos != std::string::npos; pos = line.find(q, pos + 1)) {
            if (only_blanks_or_comment(line, pos + 1)) {
                closing = pos;
                // A further quote inside the comment suggests that the value continued past the chosen quote
                ambiguous = line.find(q, pos + 1) != std::string::npos;
                break;
            }
        }
    }

    if (closing == std::string::npos) {
        closing = line.rfind(q);
        if (closing < begin) {
            closing = line.size(); // no closing quote; take the remainder of the line
        }
    }

    the_value = line.substr(begin, closing - begin);
    unescape_newlines(the_value);

    if (ambiguous) {
        log(Log::WAR,
            MESSAGE("Label::parse: ambiguous label line, the value may be truncated; label '"
                    << the_name << "' read with default value '" << the_value << "' and current value '"
                    << the_new_value << "' from: " << line));
    }
}

template <class Archive>
void Label::serialize(Archive& ar) {
    ar(CEREAL_NVP(n_));
    CEREAL_OPTIONAL_NVP(ar, v_, [this]() { return !v_.empty(); });
    CEREAL_OPTIONAL_NVP(ar, new_v_, [this]() { return !new_v_.empty(); }); // conditionally save
}

template <class Archive>
void Event::serialize(Archive& ar) {
    CEREAL_OPTIONAL_NVP(ar, n_, [this]() { return !n_.empty(); });
    CEREAL_OPTIONAL_NVP(ar, number_, [this]() { return number_ != std::numeric_limits<int>::max(); });
    CEREAL_OPTIONAL_NVP(ar, v_, [this]() { return v_; });
    CEREAL_OPTIONAL_NVP(ar, iv_, [this]() { return iv_; });
}

template <class Archive>
void Meter::serialize(Archive& ar) {
    ar(CEREAL_NVP(min_), CEREAL_NVP(max_), CEREAL_NVP(v_), CEREAL_NVP(n_), CEREAL_NVP(cc_));
}

CEREAL_TEMPLATE_SPECIALIZE(Label);
CEREAL_TEMPLATE_SPECIALIZE(Event);
CEREAL_TEMPLATE_SPECIALIZE(Meter);
