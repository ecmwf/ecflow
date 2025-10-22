/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/DefsDelta.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/MiscAttrs.hpp"

using namespace ecf;
using namespace std;

// #define DEBUG_MEMENTO 1

void Node::clear() {

    // n.b. CompoundMemento relies on clearing all attributes with state

    late_.reset(nullptr);
    c_expr_.reset(nullptr);
    t_expr_.reset(nullptr);

    misc_attrs_.reset(nullptr);

    todays_.clear();
    times_.clear();
    crons_.clear();
    days_.clear();
    dates_.clear();

    //
    // n.b. AutoCancel, AutoRestore, and AutoArchive do not have changeable state.
    //      Thus, there is nothing to clear and there is no need for a memento.
    //

    meters_.clear();
    events_.clear();
    labels_.clear();

    avisos_.clear();
    mirrors_.clear();

    repeat_.clear();
    vars_.clear();
    limits_.clear();
    inLimitMgr_.clear();
}

void Node::incremental_changes(DefsDelta& changes, compound_memento_ptr& comp) const {
#ifdef DEBUG_MEMENTO
    std::cout << "Node::incremental_changes(DefsDelta& changes) " << debugNodePath() << "\n";
#endif

    unsigned int client_state_change_no = changes.client_state_change_no();

    // Create NodeState Memento to signal a change in the node state
    if (st_.first.state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeStateMemento>(std::make_pair(st_.first.state(), st_.second)));
    }

    // Create NodeDefStatusDeltaMemento to signal a change in the node default state
    if (d_st_.state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeDefStatusDeltaMemento>(d_st_.state()));
    }

    // Create SuspendedMemento to signal a change in the suspended node state
    if (suspended_change_no_ > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<SuspendedMemento>(suspended_));
    }

    // Create a memento for each attribute, whenever any single attribute is added or deleted.
    //
    // n.b. All attributes are copies, meaning that on the client side all existing attributes
    //      are replaced by the ones that are included in the mementos
    //
    if (state_change_no_ > client_state_change_no) {

        /// *****************************************************************************************
        /// Node attributes DELETED or ADDED, i.e we call comp->clear_attributes()
        /// *****************************************************************************************

#ifdef DEBUG_MEMENTO
        std::cout << "    Node::incremental_changes()    Attributes added or deleted\n";
#endif
        // Note: auto-cancel does not have any alterable state hence, *NO* memento

        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->clear_attributes();

        for (const Meter& m : meters_) {
            comp->add(std::make_shared<NodeMeterMemento>(m));
        }
        for (const Event& e : events_) {
            comp->add(std::make_shared<NodeEventMemento>(e));
        }
        for (const Label& l : labels_) {
            comp->add(std::make_shared<NodeLabelMemento>(l));
        }
        for (const auto& a : avisos_) {
            comp->add(std::make_shared<NodeAvisoMemento>(a));
        }
        for (const auto& m : mirrors_) {
            comp->add(std::make_shared<NodeMirrorMemento>(m));
        }
        for (const ecf::TodayAttr& attr : todays_) {
            comp->add(std::make_shared<NodeTodayMemento>(attr));
        }
        for (const ecf::TimeAttr& attr : times_) {
            comp->add(std::make_shared<NodeTimeMemento>(attr));
        }
        for (const DayAttr& attr : days_) {
            comp->add(std::make_shared<NodeDayMemento>(attr));
        }
        for (const DateAttr& attr : dates_) {
            comp->add(std::make_shared<NodeDateMemento>(attr));
        }
        for (const CronAttr& attr : crons_) {
            comp->add(std::make_shared<NodeCronMemento>(attr));
        }

        if (misc_attrs_) {
            const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
            if (!verify_attrs.empty()) {
                comp->add(std::make_shared<NodeVerifyMemento>(verify_attrs));
            }

            const std::vector<ZombieAttr>& zombie_attrs = misc_attrs_->zombies();
            for (const ZombieAttr& attr : zombie_attrs) {
                comp->add(std::make_shared<NodeZombieMemento>(attr));
            }

            const std::vector<QueueAttr>& queue_attrs = misc_attrs_->queues();
            for (const QueueAttr& attr : queue_attrs) {
                comp->add(std::make_shared<NodeQueueMemento>(attr));
            }

            const std::vector<GenericAttr>& generic_attrs = misc_attrs_->generics();
            for (const GenericAttr& attr : generic_attrs) {
                comp->add(std::make_shared<NodeGenericMemento>(attr));
            }
        }

        for (const limit_ptr& l : limits_) {
            comp->add(std::make_shared<NodeLimitMemento>(*l));
        }
        for (const Variable& v : vars_) {
            comp->add(std::make_shared<NodeVariableMemento>(v));
        }

        inLimitMgr_.get_memento(comp);

        if (t_expr_) {
            comp->add(std::make_shared<NodeTriggerMemento>(*t_expr_));
        }
        if (c_expr_) {
            comp->add(std::make_shared<NodeCompleteMemento>(*c_expr_));
        }
        if (!repeat_.empty()) {
            comp->add(std::make_shared<NodeRepeatMemento>(repeat_));
        }
        if (late_) {
            comp->add(std::make_shared<NodeLateMemento>(*late_));
        }

        comp->add(std::make_shared<FlagMemento>(flag_));

        changes.add(comp);
        return;
    }

    /// *****************************************************************************************
    /// Node attributes CHANGED
    /// *****************************************************************************************

    // ** if start to Change ZombieAttr then it needs to be added here, currently we only add/delete.

    // Create NodeEventMemento to signal a change in a node event
    for (const Event& e : events_) {
        if (e.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeEventMemento>(e));
        }
    }

    // Create NodeMeterMemento to signal a change in a node meter
    for (const Meter& m : meters_) {
        if (m.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeMeterMemento>(m));
        }
    }

    // Create NodeLabelMemento to signal a change in a node label
    for (const Label& l : labels_) {
        if (l.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeLabelMemento>(l));
        }
    }

    // Create NodeAvisoMemento to signal a change in a node aviso
    for (const auto& a : avisos_) {
        if (a.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeAvisoMemento>(a));
        }
    }

    // Create NodeMirrorMemento to signal a change in a node mirror
    for (const auto& a : mirrors_) {
        if (a.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeMirrorMemento>(a));
        }
    }

    // Create NodeTodayMemento to signal a change in a node today
    for (const TodayAttr& attr : todays_) {
        if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeTodayMemento>(attr));
        }
    }

    // Create NodeTimeMemento to signal a change in a node time
    for (const TimeAttr& attr : times_) {
        if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeTimeMemento>(attr));
        }
    }

    // Create NodeDayMemento to signal a change in a node day
    for (const DayAttr& attr : days_) {
        if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeDayMemento>(attr));
        }
    }

    // Create NodeDateMemento to signal a change in a node date
    for (const DateAttr& attr : dates_) {
        if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeDateMemento>(attr));
        }
    }

    // Create NodeCronMemento to signal a change in a node cron
    for (const CronAttr& attr : crons_) {
        if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeCronMemento>(attr));
        }
    }

    if (misc_attrs_) {

        // Create NodeQueueIndexMemento to signal a change in a node queue index
        for (const QueueAttr& attr : misc_attrs_->queues()) {
            if (attr.state_change_no() > client_state_change_no) {
                if (!comp.get()) {
                    comp = std::make_shared<CompoundMemento>(absNodePath());
                }
                comp->add(std::make_shared<NodeQueueIndexMemento>(attr.name(), attr.index(), attr.state_vec()));
            }
        }

        // Zombies have no state that changes, so no NodeZombieMemento is created here!

        // Create NodeVerifyMemento to signal a change in a node verify
        //
        // n.b. In this case, if one verify changes then the memento will copy all of
        //      them as this avoids having to work out which one actually changed.
        const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
        for (const VerifyAttr& v : verify_attrs) {
            if (v.state_change_no() > client_state_change_no) {
                if (!comp.get()) {
                    comp = std::make_shared<CompoundMemento>(absNodePath());
                }
                comp->add(std::make_shared<NodeVerifyMemento>(verify_attrs));
                break;
            }
        }
    }

    // Create NodeTriggerMemento to signal a change in the node trigger expression
    if (t_expr_ && t_expr_->state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeTriggerMemento>(*t_expr_));
    }

    // Create NodeCompleteMemento to signal a change in the node complete expression
    if (c_expr_ && c_expr_->state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeCompleteMemento>(*c_expr_));
    }

    // Create NodeRepeatIndexMemento to signal a change in the node repeat
    if (!repeat_.empty() && repeat_.state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeRepeatIndexMemento>(repeat_));
    }

    // Create NodeLimitMemento to signal a change in the node limit
    for (const limit_ptr& l : limits_) {
        if (l->state_change_no() > client_state_change_no) {
            if (!comp.get()) {
                comp = std::make_shared<CompoundMemento>(absNodePath());
            }
            comp->add(std::make_shared<NodeLimitMemento>(*l));
        }
    }

    // Create NodeVariableMemento to signal a change in the node variables
    //
    // n.b. if anything changes, all variables are included in the memento
    //      to replace on the recipient node
    if (variable_change_no_ > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        for (const Variable& v : vars_) {
            comp->add(std::make_shared<NodeVariableMemento>(v));
        }
    }

    // Create NodeLateMemento to signal a change in the node late
    if (late_ && late_->state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<NodeLateMemento>(*late_));
    }

    // Create FlagMemento to signal a change in the node flag
    if (flag_.state_change_no() > client_state_change_no) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<FlagMemento>(flag_));
    }

    if (comp.get()) {
        changes.add(comp);
    }
}

void Node::set_memento(const NodeStateMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const StateMemento* memento) " << debugNodePath() << "  "
              << NState::toString(memento->state_) << "\n";
#endif

    if (aspect_only) {
        aspects.push_back(ecf::Aspect::STATE);
        return;
    }

    setStateOnly(memento->state_.first);
    st_.second = memento->state_.second;
}

void Node::set_memento(const NodeDefStatusDeltaMemento* memento,
                       std::vector<ecf::Aspect::Type>& aspects,
                       bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeDefStatusDeltaMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        aspects.push_back(ecf::Aspect::DEFSTATUS);
        return;
    }

    d_st_.setState(memento->state_);
}

void Node::set_memento(const SuspendedMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const SuspendedMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        aspects.push_back(ecf::Aspect::SUSPENDED);
        return;
    }

    if (memento->suspended_) {
        suspend();
    }
    else {
        resume();
    }
}

void Node::set_memento(const NodeEventMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeEventMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::EVENT);
        return;
    }

    // Attempt to update an existing event
    if (set_event(memento->event_.name_or_number(), memento->event_.value())) {
        return;
    }

    // Otherwise, add a new event
    addEvent(memento->event_);
}

void Node::set_memento(const NodeMeterMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeMeterMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::METER);
        return;
    }

    // Attempt to update an existing meter
    if (set_meter(memento->meter_.name(), memento->meter_.value())) {
        return;
    }

    // Otherwise, add a new meter
    addMeter(memento->meter_);
}

void Node::set_memento(const NodeLabelMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeLabelMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::LABEL);
        return;
    }

    // Attempt to update an existing label
    for (auto& label : labels_) {
        if (label.name() == memento->label_.name()) {
            label = memento->label_;
            return;
        }
    }

    // Otherwise, add a new label
    addLabel(memento->label_);
}

void Node::set_memento(const NodeAvisoMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeAvisoMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::AVISO);
        return;
    }

    // Attempt to update an existing aviso
    for (auto& aviso : avisos_) {
        if (aviso.name() == memento->aviso_.name()) {
            aviso = memento->aviso_;
            return;
        }
    }

    // Otherwise, add a new aviso
    addAviso(memento->aviso_);
}

void Node::set_memento(const NodeMirrorMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeMirrorMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::MIRROR);
        return;
    }

    // Attempt to update an existing mirror
    for (auto& mirror : mirrors_) {
        if (mirror.name() == memento->mirror_.name()) {
            mirror = memento->mirror_;
            return;
        }
    }

    // Otherwise, add a new mirror
    addMirror(memento->mirror_);
}

void Node::set_memento(const NodeQueueMemento* m, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::QUEUE);
        return;
    }

    // Attempt to update an existing queue
    if (misc_attrs_) {
        misc_attrs_->set_memento(m);
        return;
    }

    // Otherwise, add a new queue
    add_queue(m->queue_);
}

void Node::set_memento(const NodeGenericMemento* m, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::GENERIC);
        return;
    }

    // Attempt to update an existing generic
    if (misc_attrs_) {
        misc_attrs_->set_memento(m);
        return;
    }

    // Otherwise, add a new generic
    add_generic(m->generic_);
}

void Node::set_memento(const NodeQueueIndexMemento* m, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
    if (aspect_only) {
        // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::QUEUE_INDEX);
        return;
    }

    // Update the existing queue index
    if (misc_attrs_) {
        misc_attrs_->set_memento(m);
    }
}

void Node::set_memento(const NodeTriggerMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeTriggerMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::EXPR_TRIGGER);
        return;
    }

    // Attempt to update the existing trigger expression
    if (t_expr_) {
        if (memento->exp_.isFree()) {
            freeTrigger();
        }
        else {
            clearTrigger();
        }
        return;
    }

    // Otherwise, add a new trigger expression
    add_trigger_expression(memento->exp_);
}

void Node::set_memento(const NodeCompleteMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeCompleteMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::EXPR_COMPLETE);
        return;
    }

    // Attempt to update the existing complete expression
    if (c_expr_) {
        if (memento->exp_.isFree()) {
            freeComplete();
        }
        else {
            clearComplete();
        }
        return;
    }

    // Otherwise, add a new complete expression
    add_complete_expression(memento->exp_);
}

void Node::set_memento(const NodeRepeatMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeRepeatMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::REPEAT);
        return;
    }

    // Attempt to update the existing repeat
    if (!repeat_.empty()) {

        // Note: the node is incremented one past the last value.
        // In Node, we increment() then check for validity so the_new_value may be outside the valid range.
        // This may happen when doing an incremental sync, *hence*, allow memento to copy the value as is.
        repeat_.set_value(memento->repeat_.index_or_value());

        // Alternative, but expensive since relies on cloning and coping potentially very large vectors
        // repeat_ = memento->repeat_;
        return;
    }

    // Otherwise, add a new repeat
    addRepeat(memento->repeat_);
}

void Node::set_memento(const NodeRepeatIndexMemento* memento,
                       std::vector<ecf::Aspect::Type>& aspects,
                       bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeRepeatIndexMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::REPEAT_INDEX);
        return;
    }

    // Update the index of the current repeat
    if (!repeat_.empty()) {

        // Note: the node is incremented one past the last value.
        // In Node we increment() then check for validity so the_new_value may be outside the valid range.
        // This may happen when doing an incremental sync, *hence*, allow memento to copy the value as is.
        repeat_.set_value(memento->index_or_value_);
    }
}

void Node::set_memento(const NodeLimitMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeLimitMemento* memento) " << debugNodePath() << "  "
              << memento->limit_.toString() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::LIMIT);
        return;
    }

    // Attempt to update the existing limit
    if (limit_ptr limit = find_limit(memento->limit_.name()); limit.get()) {
        limit->set_state(memento->limit_.theLimit(), memento->limit_.value(), memento->limit_.paths());
        return;
    }

    // Otherwise, add a new limit
    addLimit(memento->limit_);
}

void Node::set_memento(const NodeInLimitMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeInLimitMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // ADD_REMOVE_ATTR aspect only, since no state
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        return;
    }

    // Add a new inlimit
    addInLimit(memento->inlimit_);
}

void Node::set_memento(const NodeVariableMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeVariableMemento* memento) " << debugNodePath() << "\n";
#endif

    // If we have added/delete variables then ecf::Aspect::ADD_REMOVE_ATTR has already been added to aspects
    if (aspect_only) {
        aspects.push_back(ecf::Aspect::NODE_VARIABLE);
        return;
    }

    // Attempt to update an existing variable
    for (auto& var : vars_) {
        if (var.name() == memento->var_.name()) {
            var.set_value(memento->var_.theValue());
            return;
        }
    }

    // Otherwise, add a new variable
    addVariable(memento->var_);
}

void Node::set_memento(const NodeLateMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeLateMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::LATE);
        return;
    }

    // Attempt to update an existing late
    if (late_) {
        late_->setLate(memento->late_.isLate());
        return;
    }

    // Otherwise, add a new late
    addLate(memento->late_);
}

void Node::set_memento(const NodeTodayMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeTodayMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::TODAY);
        return;
    }

    // Attempt to update an existing today
    for (auto& today : todays_) {
        // We need to ignore state changes in TodayAttr, (ie we don't use equality operator)
        // otherwise today will never compare
        if (today.structureEquals(memento->attr_)) {
            today = memento->attr_; // need to copy over time series state
            return;
        }
    }

    // Otherwise, add a new today
    addToday(memento->attr_);
}

void Node::set_memento(const NodeTimeMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeTimeMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::TIME);
        return;
    }

    // Attempt to update an existing time
    for (auto& time : times_) {
        // We need to ignore state changes in TimeAttr, (ie we don't use equality operator)
        // otherwise time will never compare
        if (time.structureEquals(memento->attr_)) {
            time = memento->attr_; // need to copy over time series state
            return;
        }
    }

    // Otherwise, add a new time
    addTime(memento->attr_);
}

void Node::set_memento(const NodeDayMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeDayMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::DAY);
        return;
    }

    // Attempt to update an existing day
    for (auto& day : days_) {
        // We need to ignore state changes (ie we don't use equality operator)
        // otherwise attributes will never compare
        if (day.structureEquals(memento->attr_)) {
            day = memento->attr_;
            return;
        }
    }

    // Otherwise, add a new day
    addDay(memento->attr_);
}

void Node::set_memento(const NodeDateMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeDateMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::DATE);
        return;
    }

    // Attempt to update an existing date
    for (auto& date : dates_) {
        // We need to ignore state changes (ie we don't use equality operator)
        // otherwise attributes will never compare
        if (date.structureEquals(memento->attr_)) {
            if (memento->attr_.isSetFree()) {
                date.setFree();
            }
            else {
                date.clearFree();
            }
            return;
        }
    }

    // Otherwise, add a new date
    addDate(memento->attr_);
}

void Node::set_memento(const NodeCronMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeCronMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        aspects.push_back(ecf::Aspect::CRON);
        return;
    }

    // Attempt to update an existing cron
    for (auto& cron : crons_) {
        // We need to ignore state changes (ie we don't use equality operator)
        // otherwise attributes will never compare
        if (cron.structureEquals(memento->attr_)) {
            cron = memento->attr_; // need to copy over time series state
            return;
        }
    }

    // Otherwise, add a new cron
    addCron(memento->attr_);
}

void Node::set_memento(const FlagMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const FlagMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        aspects.push_back(ecf::Aspect::FLAG);
        return;
    }

    flag_.set_flag(memento->flag_.flag());
}

void Node::set_memento(const NodeZombieMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeZombieMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        // No state, only ADD_REMOVE_ATTR aspect
        return;
    }

    // Zombie attributes should always be via ADD_REMOVE_ATTR
    // See Node::incremental_changes
    // Since there is no state to change

    /// remove existing attribute of same type, as duplicate of same type not allowed
    delete_zombie(memento->attr_.zombie_type());
    addZombie(memento->attr_);
}

void Node::set_memento(const NodeVerifyMemento* memento, std::vector<ecf::Aspect::Type>& aspects, bool aspect_only) {

#ifdef DEBUG_MEMENTO
    std::cout << "Node::set_memento(const NodeVerifyMemento* memento) " << debugNodePath() << "\n";
#endif

    if (aspect_only) {
        // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
        return;
    }

    // Ensure miscellaneous attribute exists
    if (!misc_attrs_) {
        misc_attrs_ = std::make_unique<MiscAttrs>(this);
    }

    misc_attrs_->verifys_ = memento->verifys_;
}
