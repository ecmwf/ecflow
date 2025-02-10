/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_DState_HPP
#define ecflow_core_DState_HPP

///
/// \brief DState stores the state of a node.
///
/// The class DState is used to define the enum.
/// The number of state changes in stored in `state_change_no`.
///

#include <string>

#include "ecflow/core/NState.hpp"

class DState {
public:
    enum State { UNKNOWN = 0, COMPLETE = 1, QUEUED = 2, ABORTED = 3, SUBMITTED = 4, ACTIVE = 5, SUSPENDED = 6 };

    explicit DState(State s) : st_(s), state_change_no_(0) {}
    DState() : st_(default_state()) {}
    static DState::State default_state() { return DState::QUEUED; } // NEVER change, or will break client/server

    State state() const { return st_; }
    void setState(State);

    // The state_change_no is never reset. Must be incremented if it can affect equality
    unsigned int state_change_no() const { return state_change_no_; }

    bool operator==(const DState& rhs) const { return st_ == rhs.st_; }
    bool operator!=(const DState& rhs) const { return st_ != rhs.st_; }
    bool operator==(State s) const { return s == st_; }
    bool operator!=(State s) const { return s != st_; }

    static NState::State convert(DState::State);
    static const char* toString(DState::State);
    static std::string to_html(DState::State);
    static const char* toString(const DState& ns) { return toString(ns.state()); }
    static std::string to_string(DState::State s) { return std::string(toString(s)); }
    static DState::State toState(const std::string&);
    static bool isValid(const std::string&);
    static std::vector<std::string> allStates();
    static std::vector<DState::State> states();

private:
    State st_;
    unsigned int state_change_no_{0}; // *not* persisted, only used on server side

    // *IMPORTANT* no version for a simple class
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

////////////////////////////////////////////////////////////////////////////////////////
// Thin wrapper over DState, to aid python. i.e Task("t").add(Defstatus(DState.complete))
class Defstatus {
public:
    explicit Defstatus(DState::State state) : st_(state) {}
    explicit Defstatus(const std::string& ds) : st_(DState::toState(ds)) {}
    DState::State state() const { return st_; }
    std::string to_string() const { return DState::to_string(st_); }

private:
    DState::State st_;
};

#endif /* ecflow_core_DState_HPP */
