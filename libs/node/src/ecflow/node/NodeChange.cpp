/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Stl.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/AvisoParser.hpp"
#include "ecflow/node/parser/MirrorParser.hpp"

using namespace ecf;

void Node::changeVariable(const std::string& name, const std::string& value) {
    auto found = ecf::algorithm::find_by_name(vars_, name);

    if (found == std::end(vars_)) {
        throw std::runtime_error("Node::changeVariable: Could not find variable " + name);
    }

    LOG(Log::MSG,
        "Variable at " + this->absNodePath() + ":" + found->name() + " updating from '" + found->value() + "' to '" +
            value + "'");

    found->set_value(value);
    variable_change_no_ = Ecf::incr_state_change_no();
}

bool Node::set_event(const std::string& event_name_or_number, bool value) {
    if (events_.empty()) {
        return false;
    }

    // find by name first
    {
        auto found = ecf::algorithm::find_by_name(events_, event_name_or_number);
        if (found != std::end(events_)) {
            found->set_value(value);
            return true;
        }
    }

    // Test for numeric, and then casting, is ****faster***** than relying on exception alone
    if (event_name_or_number.find_first_of(ecf::string_constants::numeric_chars) == 0) {
        try {
            auto number = ecf::convert_to<int>(event_name_or_number);
            auto found  = ecf::algorithm::find_by_number(events_, number);
            if (found != std::end(events_)) {
                found->set_value(value);
                return true;
            }
        }
        catch (const ecf::bad_conversion&) {
        }
    }
    return false;
}

bool Node::set_event_used_in_trigger(const std::string& event_name_or_number) {
    if (events_.empty()) {
        return false;
    }

    // find by name first
    {
        auto found = ecf::algorithm::find_by_name(events_, event_name_or_number);
        if (found != std::end(events_)) {
            found->usedInTrigger(true);
            return true;
        }
    }

    // Test for numeric, and then casting, is ****faster***** than relying on exception alone
    if (event_name_or_number.find_first_of(ecf::string_constants::numeric_chars) == 0) {
        try {
            auto number = ecf::convert_to<int>(event_name_or_number);
            auto found  = ecf::algorithm::find_by_number(events_, number);
            if (found != std::end(events_)) {
                found->usedInTrigger(true);
                return true;
            }
        }
        catch (const ecf::bad_conversion&) {
        }
    }

    return false;
}
void Node::changeEvent(const std::string& event_name_or_number, const std::string& setOrClear) {
    bool value;
    if (!setOrClear.empty()) {
        if (setOrClear != Event::SET() && setOrClear != Event::CLEAR()) {
            throw std::runtime_error("Node::changeEvent: Expected empty string, 'set' or 'clear' but found " +
                                     setOrClear + " for event " + event_name_or_number);
        }
        value = (setOrClear == Event::SET());
    }
    else {
        value = true;
    }

    changeEvent(event_name_or_number, value);
}

void Node::changeEvent(const std::string& event_name_or_number, bool value) {
    if (set_event(event_name_or_number, value)) {
        return;
    }
    throw std::runtime_error("Node::changeEvent: Could not find event " + event_name_or_number);
}

bool Node::set_meter(const std::string& name, int value) {
    auto found = ecf::algorithm::find_by_name(meters_, name);

    if (found == std::end(meters_)) {
        return false;
    }

    found->set_value(value);
    return true;
}
bool Node::set_meter_used_in_trigger(const std::string& name) {
    auto found = ecf::algorithm::find_by_name(meters_, name);

    if (found == std::end(meters_)) {
        return false;
    }

    found->usedInTrigger(true);
    return true;
}
void Node::changeMeter(const std::string& meter_name, const std::string& value) {
    int theValue = 0;
    try {
        theValue = ecf::convert_to<int>(value);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("Node::changeMeter expected integer value but found " + value);
    }
    changeMeter(meter_name, theValue);
}

void Node::changeMeter(const std::string& meter_name, int value) {
    if (set_meter(meter_name, value)) {
        return;
    }
    throw std::runtime_error("Node::changeMeter: Could not find meter " + meter_name);
}

void Node::changeLabel(const std::string& name, const std::string& value) {
    auto found = ecf::algorithm::find_by_name(labels_, name);

    if (found == std::end(labels_)) {
        throw std::runtime_error("Node::changeLabel: Could not find label " + name);
    }

    found->set_new_value(value);
}

void Node::changeAviso(const std::string& name, const std::string& value) {
    auto found = ecf::algorithm::find_by_name(avisos_, name);

    if (found == std::end(avisos_)) {
        throw std::runtime_error("Node::changeAviso: Could not find aviso " + name);
    }

    if (value == AvisoAttr::reload_option_value) {
        found->reload();
    }
    else {
        *found = AvisoParser::parse_aviso_line(value, name);
    }

    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::changeAviso(const std::string& name, const std::string& value, uint64_t revision) {
    auto found = ecf::algorithm::find_by_name(avisos_, name);

    if (found == std::end(avisos_)) {
        throw std::runtime_error("Node::changeAviso: Could not find aviso " + name);
    }

    auto attr = AvisoParser::parse_aviso_line(value, name);
    attr.set_revision(revision);

    // The following delete/add enforces the reconfiguration of the attribute backend thread
    this->deleteAviso(name); // delete the aviso if it exists (to avoid duplicates
    this->addAviso(attr);

    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::changeMirror(const std::string& name, const std::string& value) {
    auto found = ecf::algorithm::find_by_name(mirrors_, name);

    if (found == std::end(mirrors_)) {
        throw std::runtime_error("Node::changeMirror: Could not find mirror " + name);
    }

    if (value == MirrorAttr::reload_option_value) {
        found->reload();
    }
    else {
        auto attr = MirrorParser::parse_mirror_line(value, name, this);

        // The following delete/add enforces the reconfiguration of the attribute backend thread
        this->deleteMirror(name); // delete the mirror if it exists (to avoid duplicates
        this->addMirror(attr);
    }

    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::changeTrigger(const std::string& expression) {
    (void)parse_and_check_expressions(expression, true /*trigger*/, "Node::changeTrigger:"); // will throw for errors

    LOG(Log::MSG,
        "Trigger at " + this->absNodePath() + " updating from '" + this->basicTriggerExpression() + "' to '" +
            expression + "'");

    deleteTrigger();
    add_trigger(expression);
}

void Node::changeComplete(const std::string& expression) {
    (void)parse_and_check_expressions(expression, false /*complete*/, "Node::changeComplete:"); // will throw for errors

    LOG(Log::MSG,
        "Complete at " + this->absNodePath() + " updating from '" + this->basicCompleteExpression() + "' to '" +
            expression + "'");

    deleteComplete();
    add_complete(expression);
}

void Node::changeRepeat(const std::string& value) {
    if (repeat_.empty()) {
        throw std::runtime_error("Node::changeRepeat: Could not find repeat on " + absNodePath());
    }
    repeat_.change(value); // this can throw std::runtime_error
}

void Node::increment_repeat() {
    if (repeat_.empty()) {
        throw std::runtime_error("Node::increment_repeat: Could not find repeat on " + absNodePath());
    }
    repeat_.increment();
}

void Node::changeLimitMax(const std::string& name, const std::string& maxValue) {
    int theValue = 0;
    try {
        theValue = ecf::convert_to<int>(maxValue);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("Node::changeLimitMax expected integer value but found " + maxValue);
    }
    changeLimitMax(name, theValue);
}

void Node::changeLimitMax(const std::string& name, int maxValue) {
    limit_ptr limit = find_limit(name);
    if (!limit.get()) {
        throw std::runtime_error("Node::changeLimitMax: Could not find limit " + name);
    }
    limit->setLimit(maxValue);
}

void Node::changeLimitValue(const std::string& name, const std::string& value) {
    // The special value 'reset' requests the reset of the limit with the nodes
    // that are currently consuming it, and is therefore not expected to be an integer.
    if (value == Limit::reset_keyword) {
        reset_limit_value(name);
        return;
    }

    int theValue = 0;
    try {
        theValue = ecf::convert_to<int>(value);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("Node::changeLimitValue expected an integer value, or '" +
                                 std::string(Limit::reset_keyword) + "', but found " + value);
    }
    changeLimitValue(name, theValue);
}

void Node::changeLimitValue(const std::string& name, int value) {
    limit_ptr limit = find_limit(name);
    if (!limit.get()) {
        throw std::runtime_error("Node::changeLimitValue: Could not find limit " + name);
    }
    limit->setValue(value);
}

namespace {

///
/// @brief Visits, in pre-order, the given Node and all its descendants, including the aliases of a Task.
///
/// @param[in] node The root of the traversal
/// @param[in] visitor The function called on each visited Node
///
template <typename Visitor>
void visit_node_and_descendants(Node& node, Visitor& visitor) {
    visitor(node);

    if (auto* container = dynamic_cast<NodeContainer*>(&node); container) {
        for (const auto& child : container->children()) {
            visit_node_and_descendants(*child, visitor);
        }
    }
    else if (auto* task = dynamic_cast<Task*>(&node); task) {
        for (const auto& alias : task->aliases()) {
            visit_node_and_descendants(*alias, visitor);
        }
    }
}

} // namespace

void Node::reset_limit_value(const std::string& name) {
    limit_ptr limit = find_limit(name);
    if (!limit.get()) {
        throw std::runtime_error("Node::reset_limit_value: Could not find limit " + name);
    }

    Defs* the_defs = defs();
    if (!the_defs) {
        throw std::runtime_error("Node::reset_limit_value: Could not access the definition holding limit " + name);
    }

    // Discard the consumption currently recorded by the Limit.
    limit->reset();

    // Rebuild the consumption, by replaying the rules applied by Submittable::update_limits() for every node
    // that is currently consuming, i.e. that is in the submitted or active state. The whole definition is
    // traversed, since an inlimit is allowed to reference a Limit held in another suite.
    //
    // The 'incremented' flag of the referencing inlimits must be cleared as well, otherwise the inlimits that
    // limit a single node (i.e. -n) refuse to consume a token during the replay. The flag is cleared as part
    // of the same traversal: an inlimit is only ever consumed by the Node holding it or by a descendant (see
    // Node::incrementInLimit(), which walks up the parent hierarchy), and the traversal is pre-order, and
    // hence the flag of a Node is always cleared before any of its descendants replays.
    auto visitor = [&limit](Node& node) {
        node.inLimitMgr_.reset_for(limit.get());

        auto* submittable = dynamic_cast<Submittable*>(&node);
        if (!submittable) {
            return;
        }

        NState::State state = submittable->state();
        if (state != NState::SUBMITTED && state != NState::ACTIVE) {
            return;
        }

        {
            std::set<Limit*> limitSet; // ensure local inlimit have preference over parent
            submittable->incrementInLimit(limitSet, limit.get());
        }
        if (state == NState::ACTIVE) {
            // An active node has given up the tokens held on the inlimits that limit submission only (i.e. -s)
            std::set<Limit*> limitSet;
            submittable->decrementInLimitForSubmission(limitSet, limit.get());
        }
    };

    for (const auto& suite : the_defs->suites()) {
        visit_node_and_descendants(*suite, visitor);
    }
}

void Node::changeDefstatus(const std::string& theState) {
    if (!DState::isValid(theState)) {
        throw std::runtime_error("Node::changeDefstatus expected a state but found " + theState);
    }

    // Updates state_change_no on the defStatus
    d_st_.setState(DState::toState(theState));
}

void Node::changeLate(const ecf::LateAttr& late) {
    late_            = std::make_unique<ecf::LateAttr>(late);
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::change_time(const std::string& old, const std::string& new_time) {
    TimeAttr old_attr(TimeSeries::create(old));      // can throw if parse fails
    TimeAttr new_attr(TimeSeries::create(new_time)); // can throw if parse fails

    // Do not use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(times_, [&](const auto& item) { return item.structureEquals(old_attr); });

    if (found == std::end(times_)) {
        throw std::runtime_error("Node::change_time : Cannot find time attribute: ");
    }

    *found           = new_attr;
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::change_today(const std::string& old, const std::string& new_time) {
    TodayAttr old_attr(TimeSeries::create(old));      // can throw if parse fails
    TodayAttr new_attr(TimeSeries::create(new_time)); // can throw if parse fails

    // Do not use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(todays_, [&](const auto& item) { return item.structureEquals(old_attr); });

    if (found == std::end(todays_)) {
        throw std::runtime_error("Node::change_today : Cannot find time attribute: ");
    }

    *found           = new_attr;
    state_change_no_ = Ecf::incr_state_change_no();
}
