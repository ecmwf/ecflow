/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/attribute/QueueAttr.hpp"

#include <stdexcept>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace std;
using namespace ecf;

/////////////////////////////////////////////////////////////////////////////////////////////

const QueueAttr& QueueAttr::EMPTY() {
    static const QueueAttr queueAttr = QueueAttr();
    return queueAttr;
}
QueueAttr& QueueAttr::EMPTY1() {
    static QueueAttr queueAttr = QueueAttr();
    return queueAttr;
}

QueueAttr::QueueAttr(const std::string& name, const std::vector<std::string>& theQueue)
    : theQueue_(theQueue),
      name_(name) {
    string msg;
    if (!Str::valid_name(name, msg)) {
        throw std::runtime_error("QueueAttr::QueueAttr: Invalid queue name : " + msg);
    }
    if (theQueue.empty()) {
        throw std::runtime_error("QueueAttr::QueueAttr: No queue items specified");
    }
    for (size_t i = 0; i < theQueue.size(); i++)
        state_vec_.push_back(NState::QUEUED);
}

QueueAttr::~QueueAttr() = default;

bool QueueAttr::operator==(const QueueAttr& rhs) const {
    if (name_ != rhs.name_)
        return false;
    if (theQueue_ != rhs.theQueue_)
        return false;
    if (state_vec_ != rhs.state_vec_)
        return false;
    if (currentIndex_ != rhs.currentIndex_)
        return false;
    return true;
}

std::string QueueAttr::value() const {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theQueue_.size())) {
        return theQueue_[currentIndex_];
    }
    return "<NULL>";
}

int QueueAttr::index_or_value() const {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theQueue_.size())) {
        try {
            return ecf::convert_to<int>(theQueue_[currentIndex_]);
        }
        catch (ecf::bad_conversion&) {
            // Ignore and return currentIndex_
        }
    }
    return currentIndex_;
}

NState::State QueueAttr::state(const std::string& step) const {
    for (size_t i = 0; i < theQueue_.size(); i++) {
        if (step == theQueue_[i]) {
            if (i >= state_vec_.size())
                throw std::runtime_error("QueueAttr::state: index out of range");
            return state_vec_[i];
        }
    }
    throw std::runtime_error("QueueAttr::state: could not find step " + step);
    return NState::UNKNOWN;
}

void QueueAttr::requeue() {
    currentIndex_ = 0;
    for (auto& i : state_vec_)
        i = NState::QUEUED;
    incr_state_change_no();
}

std::string QueueAttr::active() {
    if (currentIndex_ >= 0 && currentIndex_ < static_cast<int>(theQueue_.size())) {
        state_vec_[currentIndex_] = NState::ACTIVE;
        std::string ret           = theQueue_[currentIndex_];
        currentIndex_++;
        incr_state_change_no();
        return ret;
    }
    return "<NULL>";
}

void QueueAttr::complete(const std::string& step) {
    for (size_t i = 0; i < theQueue_.size(); i++) {
        if (step == theQueue_[i]) {
            state_vec_[i] = NState::COMPLETE;
            incr_state_change_no();
            return;
        }
    }
    std::stringstream ss;
    ss << "QueueAttr::complete: Could not find " << step << " in queue " << name_;
    throw std::runtime_error(ss.str());
}

void QueueAttr::aborted(const std::string& step) {
    for (size_t i = 0; i < theQueue_.size(); i++) {
        if (step == theQueue_[i]) {
            state_vec_[i] = NState::ABORTED;
            incr_state_change_no();
            return;
        }
    }
    std::stringstream ss;
    ss << "QueueAttr::aborted: Could not find " << step << " in queue " << name_;
    throw std::runtime_error(ss.str());
}

std::string QueueAttr::no_of_aborted() const {
    int count = 0;
    for (auto i : state_vec_) {
        if (i == NState::ABORTED)
            count++;
    }
    if (count != 0)
        return ecf::convert_to<std::string>(count);
    return std::string();
}

void QueueAttr::reset_index_to_first_queued_or_aborted() {
    for (size_t i = 0; i < state_vec_.size(); i++) {
        if (state_vec_[i] == NState::QUEUED || state_vec_[i] == NState::ABORTED) {
            currentIndex_ = i;
            incr_state_change_no();
            break;
        }
    }
}

std::string QueueAttr::toString() const {
    std::string ret;
    write(ret);
    return ret;
}

void QueueAttr::write(std::string& ret) const {
    ret += "queue ";
    ret += name_;
    for (const auto& i : theQueue_) {
        ret += " ";
        ret += i;
    }
}

std::string QueueAttr::dump() const {
    std::stringstream ss;
    ss << toString() << " # " << currentIndex_;
    for (auto i : state_vec_)
        ss << " " << i;
    return ss.str();
}

void QueueAttr::incr_state_change_no() {
    state_change_no_ = Ecf::incr_state_change_no();
}

void QueueAttr::parse(QueueAttr& queAttr,
                      const std::string& line,
                      std::vector<std::string>& lineTokens,
                      bool parse_state) {
    size_t line_tokens_size = lineTokens.size();
    if (line_tokens_size < 3) {
        std::stringstream ss;
        ss << "QueueAttr::parse: expected at least 3 tokens, found " << line_tokens_size << " on line:" << line << "\n";
        throw std::runtime_error(ss.str());
    }

    // queue name "first" "second" "last" #   current_index state state state
    //   0     1     2        3       4   5   6
    queAttr.set_name(lineTokens[1]);

    std::vector<std::string> theEnums;
    theEnums.reserve(line_tokens_size);
    for (size_t i = 2; i < line_tokens_size; i++) {
        std::string theEnum = lineTokens[i];
        if (theEnum[0] == '#')
            break;
        Str::removeSingleQuotes(theEnum); // remove quotes, they get added back when we persist
        Str::removeQuotes(theEnum);       // remove quotes, they get added back when we persist
        theEnums.push_back(theEnum);
    }
    if (theEnums.empty())
        throw std::runtime_error("queue: has no values " + line);

    int index = 0;
    std::vector<NState::State> state_vec;

    if (parse_state) {
        // queue VARIABLE a b c d # index active complete aborted queued
        for (size_t i = 3; i < line_tokens_size; i++) {
            if (lineTokens[i] == "#" && i + 1 < line_tokens_size) {
                i++;
                index = Extract::theInt(lineTokens[i], "QueueAttr::parse, could not extract index");
                i++;
                for (; i < line_tokens_size; i++) {
                    NState::State state = NState::toState(lineTokens[i]);
                    state_vec.push_back(state);
                }
                break;
            }
        }
    }

    queAttr.set_queue(theEnums, index, state_vec);
}

void QueueAttr::set_queue(const std::vector<std::string>& theQueue,
                          int index,
                          const std::vector<NState::State>& state_vec) {
    if (theQueue.empty())
        throw std::runtime_error("QueueAttr::set_queue: No queue items specified");

    if (!state_vec.empty()) {
        if (state_vec.size() != theQueue.size()) {
            std::stringstream ss;
            ss << "QueueAttr::set_state: for queue " << name_ << " size " << theQueue.size()
               << " does not match state size " << state_vec.size();
            throw std::runtime_error(ss.str());
        }
        state_vec_ = state_vec;
    }
    else {
        for (size_t i = 0; i < theQueue.size(); i++)
            state_vec_.push_back(NState::QUEUED);
    }

    currentIndex_ = index;
    theQueue_     = theQueue;
}

void QueueAttr::set_state_vec(const std::vector<NState::State>& state_vec) {
    state_vec_ = state_vec;
    if (theQueue_.size() != state_vec_.size()) {
        std::cout << "QueueAttr::set_state_vec: for queue " << name_ << " queue size " << theQueue_.size()
                  << " not equal to state_vec size " << state_vec_.size() << "\n";
    }
}

void QueueAttr::set_name(const std::string& name) {
    string msg;
    if (!Str::valid_name(name, msg)) {
        throw std::runtime_error("QueueAttr::set_name: Invalid queue name : " + msg);
    }
    name_ = name;
}

template <class Archive>
void QueueAttr::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(theQueue_), CEREAL_NVP(state_vec_), CEREAL_NVP(name_), CEREAL_NVP(currentIndex_));
}
CEREAL_TEMPLATE_SPECIALIZE_V(QueueAttr);
