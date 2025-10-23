/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/NodeContainer.hpp"

#include <cassert>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <boost/filesystem/operations.hpp>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/DefsDelta.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/NodeState.hpp"
#include "ecflow/node/NodeTreeVisitor.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/move_peer.hpp"

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////
// #define DEBUG_FIND_NODE 1
// #define DEBUG_DEPENDENCIES 1

/////////////////////////////////////////////////////////////////////////////////////////
NodeContainer::NodeContainer(const std::string& name, bool check) : Node(name, check) {
}

NodeContainer::NodeContainer() = default;

void NodeContainer::copy(const NodeContainer& rhs) {
    for (const auto& r_n : rhs.nodes_) {
        Task* task = r_n->isTask();
        if (task) {
            task_ptr task_copy = std::make_shared<Task>(*task);
            task_copy->set_parent(this);
            nodes_.push_back(task_copy);
        }
        else {
            Family* family = r_n->isFamily();
            assert(family);
            family_ptr family_copy = std::make_shared<Family>(*family);
            family_copy->set_parent(this);
            nodes_.push_back(family_copy);
        }
    }
}

NodeContainer::NodeContainer(const NodeContainer& rhs) : Node(rhs) {
    copy(rhs);
}

NodeContainer& NodeContainer::operator=(const NodeContainer& rhs) {
    if (this != &rhs) {
        Node::operator=(rhs);
        nodes_.clear();
        copy(rhs);
        order_state_change_no_      = 0;
        add_remove_state_change_no_ = Ecf::incr_state_change_no();
    }
    return *this;
}

NodeContainer::~NodeContainer() = default;

bool NodeContainer::check_defaults() const {
    if (order_state_change_no_ != 0) {
        throw std::runtime_error("NodeContainer::check_defaults(): order_state_change_no_ != 0");
    }
    if (add_remove_state_change_no_ != 0) {
        throw std::runtime_error("NodeContainer::check_defaults(): add_remove_state_change_no_ != 0");
    }
    return Node::check_defaults();
}

void NodeContainer::accept(ecf::NodeTreeVisitor& v) {
    v.visitNodeContainer(this);
    for (const auto& n : nodes_) {
        n->accept(v);
    }
}

void NodeContainer::acceptVisitTraversor(ecf::NodeTreeVisitor& v) {
    v.visitNodeContainer(this);
}

void NodeContainer::begin() {
    restore_on_begin_or_requeue();
    Node::begin();
    for (const auto& n : nodes_) {
        n->begin();
    }
    handle_defstatus_propagation();
}

void NodeContainer::reset() {
    Node::reset();
    for (const auto& n : nodes_) {
        n->reset();
    }
}

void NodeContainer::handle_migration(const ecf::Calendar& c) {
    Node::handle_migration(c);
    for (const auto& n : nodes_) {
        n->handle_migration(c);
    }
}

void NodeContainer::requeue(Requeue_args& args, std::function<bool(Node*)> authorisation) {
    //	LOG(Log::DBG,"   " << debugType() << "::requeue() " << absNodePath() << " resetRepeats = " << resetRepeats);

    restore_on_begin_or_requeue();
    Node::requeue(args, authorisation);

    // For negative numbers, do nothing, i.e. do not clear
    if (args.clear_suspended_in_child_nodes_ >= 0) {
        args.clear_suspended_in_child_nodes_++;
    }

    // If the node `default status` == COMPLETE, we don't log the change of state when re-queueing the descendants.
    // Without this optimization, all children would change state to QUEUED, and eventually change back to COMPLETE.
    // This avoids flooding the log with unnecessary messages regarding several thousand children
    // (e.g. as it happens in operations). See ECFLOW-1239 for details.
    bool log_state_changes_descendents = (d_st_ != DState::COMPLETE);

    Node::Requeue_args largs(args.requeue_t,
                             true /* reset repeats, Moot for tasks */,
                             args.clear_suspended_in_child_nodes_,
                             args.reset_next_time_slot_,
                             true /* reset relative duration */,
                             log_state_changes_descendents);

    for (const auto& n : nodes_) {
        n->requeue(largs, authorisation);
    }

    handle_defstatus_propagation();
}

void NodeContainer::requeue_time_attrs() {
    Node::requeue_time_attrs();
    for (const auto& n : nodes_) {
        n->requeue_time_attrs();
    }
}

void NodeContainer::reset_late_event_meters() {
    Node::reset_late_event_meters();
    for (const auto& n : nodes_) {
        n->reset_late_event_meters();
    }
}

void NodeContainer::handle_defstatus_propagation() {
    if (d_st_ == DState::COMPLETE) {
        /// A defstatus of complete and *ONLY* complete should always be applied
        /// hierarchically downwards
        setStateOnlyHierarchically(NState::COMPLETE);
    }
    else if (d_st_ == DState::default_state()) {
        /// Reflect that the status of the children.
        /// *However* do NOT override the defstatus setting
        NState::State theSignificantStateOfImmediateChildren = computedState(Node::IMMEDIATE_CHILDREN);
        if (theSignificantStateOfImmediateChildren != state()) {
            setStateOnly(theSignificantStateOfImmediateChildren);
        }
    }
}

bool NodeContainer::run(JobsParam& jobsParam, bool force) {
    for (const auto& n : nodes_) {
        (void)n->run(jobsParam, force);
    }
    return jobsParam.getErrorMsg().empty();
}

void NodeContainer::kill(const std::string& /* zombie_pid, only valid for single task */) {
    for (const auto& n : nodes_) {
        n->kill();
    }
}

void NodeContainer::status() {
    for (const auto& n : nodes_) {
        // Avoid exception for top-down case, if Task is not active or submitted
        // Allows status cmd to run over more Tasks, without early exit, when some tasks are not active/submitted
        if (n->isTask() && (n->state() != NState::ACTIVE && n->state() != NState::SUBMITTED)) {
            continue;
        }
        n->status();
    }
}

bool NodeContainer::top_down_why(std::vector<std::string>& theReasonWhy, bool html_tags) const {
    bool why_found = Node::why(theReasonWhy, html_tags);
    if (!why_found) {
        for (const auto& n : nodes_) {
            if (n->top_down_why(theReasonWhy, html_tags)) {
                why_found = true;
            }
        }
    }
    return why_found;
}

void NodeContainer::incremental_changes(DefsDelta& changes, compound_memento_ptr& comp) const {

    // Create ChildrenMemento to signal removal/addition of children
    //
    // n.b. When signalling the removal/addition of children,
    //      any order change is irrelevant as the complete updated list of
    //      children is made part of the synchronisation
    if (add_remove_state_change_no_ > changes.client_state_change_no()) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }
        comp->add(std::make_shared<ChildrenMemento>(nodes_));
    }
    // Create OrderMemento to signal reordering of children
    else if (order_state_change_no_ > changes.client_state_change_no()) {
        if (!comp.get()) {
            comp = std::make_shared<CompoundMemento>(absNodePath());
        }

        std::vector<std::string> order_vec;
        order_vec.reserve(nodes_.size());
        for (const auto& n : nodes_) {
            order_vec.push_back(n->name());
        }
        comp->add(std::make_shared<OrderMemento>(order_vec));
    }

    Node::incremental_changes(changes, comp);
}

void NodeContainer::set_memento(const OrderMemento* memento,
                                std::vector<ecf::Aspect::Type>& aspects,
                                bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "NodeContainer::set_memento( const OrderMemento* ) " << debugNodePath() << "\n";
#endif
    if (aspect_only) {
        aspects.push_back(ecf::Aspect::ORDER);
        return;
    }

    // Order nodes_ according to memento ordering
    const std::vector<std::string>& order = memento->order_;
    if (order.size() != nodes_.size()) {
        // something gone wrong.
        std::cout << "NodeContainer::set_memento OrderMemento, memento.size() " << order.size()
                  << " Not the same as nodes_size() " << nodes_.size() << "\n";
        return;
    }

    std::vector<node_ptr> vec;
    vec.reserve(nodes_.size());
    for (const auto& i : order) {
        for (const auto& n : nodes_) {
            if (i == n->name()) {
                vec.push_back(n);
                break;
            }
        }
    }
    if (vec.size() != nodes_.size()) {
        std::cout << "NodeContainer::set_memento could not find all the names\n";
        return;
    }

    nodes_ = vec;
}

void NodeContainer::set_memento(const ChildrenMemento* memento,
                                std::vector<ecf::Aspect::Type>& aspects,
                                bool aspect_only) {
#ifdef DEBUG_MEMENTO
    std::cout << "NodeContainer::set_memento( const ChildrenMemento * ) " << debugNodePath() << "\n";
#endif
    if (aspect_only) {
        aspects.push_back(ecf::Aspect::ADD_REMOVE_NODE);
        return;
    }

    // setup child parent pointers
    nodes_ = memento->children_;
    for (auto& n : nodes_) {
        n->set_parent(this);
    }
}

void NodeContainer::collateChanges(DefsDelta& changes, const ecf::Ctx& ctx) const {
    /// Theres no point in traversing children if we have added/removed children
    /// since ChildrenMemento will copy all children.
    if (add_remove_state_change_no_ > changes.client_state_change_no()) {
        return;
    }

    // Traversal to children
    for (const auto& n : nodes_) {
        n->collateChanges(changes, ctx);
    }
}

void NodeContainer::order(Node* immediateChild, NOrder::Order ord) {
    SuiteChanged1 changed(suite());
    switch (ord) {
        case NOrder::TOP: {
            for (auto i = nodes_.begin(); i != nodes_.end(); ++i) {
                if ((*i).get() == immediateChild) {
                    node_ptr node = (*i);
                    nodes_.erase(i);
                    nodes_.insert(nodes_.begin(), node);
                    order_state_change_no_ = Ecf::incr_state_change_no();
                    return;
                }
            }
            throw std::runtime_error("NodeContainer::order TOP, immediate child not found");
        }
        case NOrder::BOTTOM: {
            for (auto i = nodes_.begin(); i != nodes_.end(); ++i) {
                if ((*i).get() == immediateChild) {
                    node_ptr node = (*i);
                    nodes_.erase(i);
                    nodes_.push_back(node);
                    order_state_change_no_ = Ecf::incr_state_change_no();
                    return;
                }
            }
            throw std::runtime_error("NodeContainer::order BOTTOM, immediate child not found");
        }
        case NOrder::ALPHA: {
            std::sort(nodes_.begin(), nodes_.end(), [](const node_ptr& a, const node_ptr& b) {
                try {
                    int a_as_int = ecf::convert_to<int>(a->name());
                    int b_as_int = ecf::convert_to<int>(b->name());
                    return a_as_int < b_as_int;
                }
                catch (const ecf::bad_conversion&) {
                }

                return Str::caseInsLess(a->name(), b->name());
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            break;
        }
        case NOrder::ORDER: {
            std::sort(nodes_.begin(), nodes_.end(), [](const node_ptr& a, const node_ptr& b) {
                return Str::caseInsGreater(a->name(), b->name());
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            break;
        }
        case NOrder::UP: {
            for (size_t t = 0; t < nodes_.size(); t++) {
                if (nodes_[t].get() == immediateChild) {
                    if (t != 0) {
                        node_ptr node = nodes_[t];
                        nodes_.erase(nodes_.begin() + t);
                        t--;
                        nodes_.insert(nodes_.begin() + t, node);
                        order_state_change_no_ = Ecf::incr_state_change_no();
                    }
                    return;
                }
            }
            throw std::runtime_error("NodeContainer::order UP, immediate child not found");
        }
        case NOrder::DOWN: {
            for (size_t t = 0; t < nodes_.size(); t++) {
                if (nodes_[t].get() == immediateChild) {
                    if (t != nodes_.size() - 1) {
                        node_ptr node = nodes_[t];
                        nodes_.erase(nodes_.begin() + t);
                        t++;
                        nodes_.insert(nodes_.begin() + t, node);
                        order_state_change_no_ = Ecf::incr_state_change_no();
                    }
                    return;
                }
            }
            throw std::runtime_error("NodeContainer::order DOWN, immediate child not found");
        }
        case NOrder::RUNTIME: {
            for (node_ptr node : nodes_) {
                if (node->state() != NState::COMPLETE) {
                    throw std::runtime_error("NodeContainer::order: To order by RUNTIME All nodes must be complete");
                }
            }
            (void)sum_runtime();
            std::sort(nodes_.begin(), nodes_.end(), [](const node_ptr& a, const node_ptr& b) {
                return a->state_change_runtime() > b->state_change_runtime();
            });
            order_state_change_no_ = Ecf::incr_state_change_no();
            break;
        }
    }
}

void NodeContainer::move_peer(Node* src, Node* dest) {
    move_peer_node(nodes_, src, dest, "NodeContainer");
    order_state_change_no_ = Ecf::incr_state_change_no();
}

boost::posix_time::time_duration NodeContainer::sum_runtime() {
    boost::posix_time::time_duration rt;
    for (const auto& n : nodes_) {
        rt += n->sum_runtime();
    }
    set_runtime(rt);
    return rt;
}

bool NodeContainer::calendarChanged(const ecf::Calendar& c,
                                    Node::Calendar_args& cal_args,
                                    const ecf::LateAttr* inherited_late,
                                    bool holding_parent_day_or_date) {
    // A node that is archived should not allow any change of state.
    if (get_flag().is_set(ecf::Flag::ARCHIVED)) {
        return false;
    }

    // holding_parent_day_or_date_ is used to avoid freeing time attributes, when we have a holding parent day/date
    holding_parent_day_or_date = Node::calendarChanged(c, cal_args, nullptr, holding_parent_day_or_date);

    // if (holding_parent_day_or_date)
    //    cout << " calendarChanged: " << debugNodePath() << " " << holding_parent_day_or_date << " •••••••••• \n";

    // The late attribute is inherited, we only set late on the task/alias
    LateAttr overridden_late;
    if (inherited_late && !inherited_late->isNull()) {
        overridden_late = *inherited_late;
    }
    if (late_.get() != inherited_late) {
        overridden_late.override_with(late_.get());
    }

    for (const auto& n : nodes_) {
        (void)n->calendarChanged(c, cal_args, &overridden_late, holding_parent_day_or_date);
    }
    return false;
}

bool NodeContainer::hasAutoCancel() const {
    if (Node::hasAutoCancel()) {
        return true;
    }
    for (const auto& n : nodes_) {
        if (n->hasAutoCancel()) {
            return true;
        }
    }
    return false;
}

void NodeContainer::invalidate_trigger_references() const {
    Node::invalidate_trigger_references();
    for (const auto& n : nodes_) {
        n->invalidate_trigger_references();
    }
}

bool NodeContainer::resolveDependencies(JobsParam& jobsParam) {
#ifdef DEBUG_DEPENDENCIES
    LogToCout toCoutAsWell;
#endif

    // cout << "NodeContainer::resolveDependencies " << absNodePath() << endl;
    //  Don't evaluate children unless parent is free. BOMB out early for this case.
    //  Note:: Task::resolveDependencies() will check inLimit up front. *** THIS CHECKS UP THE HIERARCHY ***
    //  Note:: Node::resolveDependencies() may have forced family node to complete, should have
    //         returned false in this case, to stop any job submission
    if (!Node::resolveDependencies(jobsParam)) {

#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG,
            "   NodeContainer::resolveDependencies " << absNodePath()
                                                     << " could not resolve dependencies, may have completed HOLDING");
#endif
        return false;
    }

    /// During *top down* traversal we check in limits at this level. Done here rather than
    /// in Node::resolveDependencies(). Otherwise this particular check will get duplicated
    /// since the task, will do *bottom up* traversal.
    if (!check_in_limit()) {
#ifdef DEBUG_DEPENDENCIES
        LOG(Log::DBG, "   NodeContainer::resolveDependencies() " << absNodePath() << " HOLDING due to inLIMIT");
#endif
        return false;
    }

    for (const auto& n : nodes_) {
        // Note: we don't bomb out early here. Since a later child could be free e.g. f1/ty or t4
        // child t1 holding
        // child t2 holding
        // child f1 free
        //   child tx holding
        //   child ty free
        // child t3 holding
        // child t4 free
        (void)n->resolveDependencies(jobsParam);
    }

    return true;
}

bool NodeContainer::has_time_based_attributes() const {
    if (Node::has_time_based_attributes()) {
        return true;
    }
    for (const auto& node : nodes_) {
        if (node->has_time_based_attributes()) {
            return true;
        }
    }
    return false;
}

NState::State NodeContainer::computedState(Node::TraverseType traverseType) const {
    if (nodes_.empty()) {
        /// Note: theComputedNodeState will return unknown if no children, in this
        /// case just return the current state.
        return state();
    }

    // returns the computed state depending on traverseType
    // If not IMMEDIATE_CHILDREN, will recurse down calling each child's computedState() function
    return ecf::theComputedNodeState(nodes_, (traverseType == Node::IMMEDIATE_CHILDREN));
}

void NodeContainer::force_sync() {
    add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

node_ptr NodeContainer::removeChild(Node* child) {
    size_t node_vec_size = nodes_.size();
    for (size_t t = 0; t < node_vec_size; t++) {
        if (nodes_[t].get() == child) {
            node_ptr node = std::dynamic_pointer_cast<Node>(nodes_[t]);
            child->set_parent(nullptr); // must set to NULL, allows it to be re-added to different parent
            nodes_.erase(nodes_.begin() + t);
            add_remove_state_change_no_ = Ecf::incr_state_change_no();
            return node;
        }
    }
    // Should never happen
    LOG_ASSERT(false, "NodeContainer::removeChild: Could not remove child");
    return node_ptr();
}

bool NodeContainer::addChild(const node_ptr& child, size_t position) {
    // *** CANT construct shared_ptr from a raw pointer, must use dynamic_pointer_cast,
    // *** otherwise the reference counts will get messed up.
    try {
        if (child->isTask()) {
            // can throw if duplicate names
            addTask(std::dynamic_pointer_cast<Task>(child), position);
            return true;
        }

        if (child->isFamily()) {
            // can throw if duplicate names
            addFamily(std::dynamic_pointer_cast<Family>(child), position);
            return true;
        }
    }
    catch (std::runtime_error& e) {
        return false; // Duplicate names, or trying to add a Suite?
    }

    // Duplicate names, or trying to add a Suite?
    return false;
}

bool NodeContainer::isAddChildOk(Node* theChild, std::string& errorMsg) const {
    Task* theTaskChild = theChild->isTask();
    if (theTaskChild) {

        node_ptr theTask = find_by_name(theChild->name());
        if (!theTask.get()) {
            return true;
        }

        std::stringstream ss;
        ss << "Task/Family of name " << theChild->name() << " already exists in container node " << name();
        errorMsg += ss.str();
        return false;
    }

    Family* theFamilyChild = theChild->isFamily();
    if (theFamilyChild) {

        node_ptr theFamily = find_by_name(theChild->name());
        if (!theFamily.get()) {
            return true;
        }

        std::stringstream ss;
        ss << "Family/Task of name " << theChild->name() << " already exists in container node " << name();
        errorMsg += ss.str();
        return false;
    }

    Suite* theSuite = theChild->isSuite();
    if (theSuite) {
        errorMsg += "Cannot add a suite as child.";
        return false;
    }

    errorMsg += "Unknown node type";
    return false;
}

void NodeContainer::handleStateChange() {
    // Increment any repeats & requeue
    requeueOrSetMostSignificantStateUpNodeTree();

    Node::handleStateChange(); // may result in an auto-restore, if state is COMPLETE
}

size_t NodeContainer::child_position(const Node* child) const {
    size_t node_vec_size = nodes_.size();
    for (size_t t = 0; t < node_vec_size; t++) {
        if (nodes_[t].get() == child) {
            return t;
        }
    }
    return std::numeric_limits<std::size_t>::max();
}

task_ptr NodeContainer::add_task(const std::string& task_name) {
    if (find_by_name(task_name).get()) {
        std::stringstream ss;
        ss << "Add Task failed: A task/family of name '" << task_name << "' already exists on node " << debugNodePath();
        throw std::runtime_error(ss.str());
    }
    task_ptr the_task = Task::create(task_name);
    add_task_only(the_task);
    return the_task;
}

family_ptr NodeContainer::add_family(const std::string& family_name) {
    if (find_by_name(family_name).get()) {
        std::stringstream ss;
        ss << "Add Family failed: A Family/Task of name '" << family_name << "' already exists on node "
           << debugNodePath();
        throw std::runtime_error(ss.str());
    }
    family_ptr the_family = Family::create(family_name);
    add_family_only(the_family);
    return the_family;
}

void NodeContainer::addTask(const task_ptr& t, size_t position) {
    if (find_by_name(t->name()).get()) {
        std::stringstream ss;
        ss << "Add Task failed: A Task/Family of name '" << t->name() << "' already exists on node " << debugNodePath();
        throw std::runtime_error(ss.str());
    }
    add_task_only(t, position);
}

void NodeContainer::add_task_only(const task_ptr& t, size_t position) {
    if (t->parent()) {
        std::stringstream ss;
        ss << debugNodePath() << ": Add Task failed: A task of name '" << t->name()
           << "' is already owned by another node";
        throw std::runtime_error(ss.str());
    }

    t->set_parent(this);
    if (position >= nodes_.size()) {
        nodes_.push_back(t);
    }
    else {
        nodes_.insert(nodes_.begin() + position, t);
    }
    add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

void NodeContainer::add_family_only(const family_ptr& f, size_t position) {
    if (f->parent()) {
        std::stringstream ss;
        ss << debugNodePath() << ": Add Family failed: A family of name '" << f->name()
           << "' is already owned by another node";
        throw std::runtime_error(ss.str());
    }

    f->set_parent(this);
    if (position >= nodes_.size()) {
        nodes_.push_back(f);
    }
    else {
        nodes_.insert(nodes_.begin() + position, f);
    }
    add_remove_state_change_no_ = Ecf::incr_state_change_no();
}

void NodeContainer::addFamily(const family_ptr& f, size_t position) {
    if (find_by_name(f->name()).get()) {
        std::stringstream ss;
        ss << "Add Family failed: A Family/Task of name '" << f->name() << "' already exists on node "
           << debugNodePath();
        throw std::runtime_error(ss.str());
    }
    add_family_only(f, position);
}

void NodeContainer::add_child(const node_ptr& child, size_t position) {
    if (child->isTask()) {
        task_ptr task_child = std::dynamic_pointer_cast<Task>(child);
        addTask(task_child, position);
    }
    else if (child->isFamily()) {
        family_ptr family_child = std::dynamic_pointer_cast<Family>(child);
        addFamily(family_child, position);
    }
}

node_ptr NodeContainer::findImmediateChild(const std::string& theName, size_t& child_pos) const {
    size_t node_vec_size = nodes_.size();
    for (size_t t = 0; t < node_vec_size; t++) {
        if (nodes_[t]->name() == theName) {
            child_pos = t;
            return nodes_[t];
        }
    }
    child_pos = std::numeric_limits<std::size_t>::max();
    return node_ptr();
}

node_ptr NodeContainer::find_immediate_child(const std::string_view& name) const {
    for (const auto& n : nodes_) {
        if (name == n->name()) {
            return n;
        }
    }
    return node_ptr();
}

node_ptr NodeContainer::find_node_up_the_tree(const std::string& the_name) const {
    if (name() == the_name) {
        return non_const_this();
    }

    size_t not_used;
    node_ptr fnd_node = findImmediateChild(the_name, not_used);
    if (fnd_node) {
        return fnd_node;
    }

    Node* the_parent = parent();
    if (the_parent) {
        return the_parent->find_node_up_the_tree(the_name);
    }
    return node_ptr();
}

node_ptr NodeContainer::find_relative_node(const std::vector<std::string>& pathToNode) {
#ifdef DEBUG_FIND_NODE
    cout << "NodeContainer::find_relative_node for '" << name() << "\n";
    cout << " path :";
    for (const std::string& s : pathToNode) {
        cout << " " << s;
    }
    cout << "\n";
    for (node_ptr t : nodes_) {
        cout << " " << t->name();
    }
    cout << "\n";
#endif
    if (pathToNode.empty()) {
        return node_ptr();
    }
    auto pathSize = static_cast<int>(pathToNode.size());

#ifdef DEBUG_FIND_NODE
    cout << "NodeContainer::find_relative_node name = '" << name() << "' pathToNode[0] = '" << pathToNode[0] << "'\n";
#endif

    // Must match all children
    int index         = 0;
    size_t child_pos  = 0; // unused
    node_ptr the_node = shared_from_this();
    while (index < pathSize) {
        the_node = the_node->findImmediateChild(pathToNode[index], child_pos);
        if (the_node) {
            if (index == pathSize - 1) {
                return the_node;
            }
            index++;
        }
        else {
            return node_ptr();
        }
    }
    return node_ptr();
}

void NodeContainer::find_closest_matching_node(const std::vector<std::string>& pathToNode,
                                               int indexIntoPathNode,
                                               node_ptr& closest_matching_node) {
    auto pathSize = static_cast<int>(pathToNode.size());
    if (indexIntoPathNode >= pathSize) {
        return;
    }

    int index = indexIntoPathNode;
    if (name() == pathToNode[indexIntoPathNode]) {

        closest_matching_node = shared_from_this();

        // Match the Container i.e. family or suite
        bool lastIndex = (indexIntoPathNode == pathSize - 1);
        if (lastIndex) {
            return;
        }

        // Match the Children, i.e. go down the hierarchy
        index++;
        match_closest_children(pathToNode, index, closest_matching_node);
    }
}

void NodeContainer::match_closest_children(const std::vector<std::string>& pathToNode,
                                           int indexIntoPathNode,
                                           node_ptr& closest_matching_node) {
    auto pathSize = static_cast<int>(pathToNode.size());
    if (indexIntoPathNode >= pathSize) {
        return;
    }

    bool lastIndex = (indexIntoPathNode == pathSize - 1);
    if (lastIndex) {
        // Even if the name matches, it is only valid if the index is the last index
        // e.g. given a suite /a/b/c/d/e
        //      and the path  /a/b/c/d/e/f/g
        // The path will match with e, but it invalid since it is not the last index
        for (const auto& n : nodes_) {
            if (n->name() == pathToNode[indexIntoPathNode]) {
                closest_matching_node = n;
                return;
            }
        }
    }
    else {
        // Path to node is of the form "/family/task" or "/family/family/task"
        // Path to node is of the form "/suite/task" or "/suite/family/task"
        for (const auto& n : nodes_) {
            Family* family = n->isFamily();
            if (family) {
                node_ptr matching_node;
                family->find_closest_matching_node(pathToNode, indexIntoPathNode, matching_node);
                if (matching_node.get()) {
                    closest_matching_node = matching_node;
                    return;
                }
            }
        }
    }
}

node_ptr NodeContainer::find_by_name(const std::string& name) const {
    for (const auto& n : nodes_) {
        if (n->name() == name) {
            return n;
        }
    }
    return node_ptr();
}

family_ptr NodeContainer::findFamily(const std::string& familyName) const {
    for (const auto& n : nodes_) {
        if (n->name() == familyName && n->isFamily()) {
            return std::dynamic_pointer_cast<Family>(n);
        }
    }
    return family_ptr();
}

task_ptr NodeContainer::findTask(const std::string& taskName) const {
    for (const auto& n : nodes_) {
        if (n->name() == taskName && n->isTask()) {
            return std::dynamic_pointer_cast<Task>(n);
        }
    }
    return task_ptr();
}

std::string NodeContainer::find_node_path(const std::string& type, const std::string& node_name) const {
    for (const auto& n : nodes_) {
        std::string res = n->find_node_path(type, node_name);
        if (!res.empty()) {
            return res;
        }
    }
    return string();
}

bool NodeContainer::hasTimeDependencies() const {
    for (const auto& n : nodes_) {
        if (n->hasTimeDependencies()) {
            return true;
        }
    }
    return false;
}

void NodeContainer::immediateChildren(std::vector<node_ptr>& theChildren) const {
    theChildren.reserve(theChildren.size() + nodes_.size());
    for (const auto& n : nodes_) {
        theChildren.push_back(n);
    }
}

void NodeContainer::allChildren(std::vector<node_ptr>& vec) const {
    for (const auto& n : nodes_) {
        vec.push_back(n);
        n->allChildren(vec); // for task does nothing
    }
}

bool NodeContainer::check(std::string& errorMsg, std::string& warningMsg) const {
    Node::check(errorMsg, warningMsg);

    // recursive to handle hierarchical families
    for (const auto& n : nodes_) {
        n->check(errorMsg, warningMsg);
        // if (!errorMsg.empty()) break;
    }

    return errorMsg.empty();
}

std::vector<task_ptr> NodeContainer::taskVec() const {
    std::vector<task_ptr> vec;
    vec.reserve(nodes_.size());
    for (const auto& n : nodes_) {
        if (n->isTask()) {
            vec.push_back(std::dynamic_pointer_cast<Task>(n));
        }
    }
    return vec;
}

std::vector<family_ptr> NodeContainer::familyVec() const {
    std::vector<family_ptr> vec;
    for (const auto& n : nodes_) {
        if (n->isFamily()) {
            vec.push_back(std::dynamic_pointer_cast<Family>(n));
        }
    }
    return vec;
}

bool NodeContainer::operator==(const NodeContainer& rhs) const {
    size_t node_vec_size = nodes_.size();
    if (node_vec_size != rhs.nodes_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "NodeContainer::operator==  node_vec_size != rhs.nodes_.size() " << absNodePath() << "\n";
            std::cout << "   nodes_.size() = " << node_vec_size << "  rhs.nodes_.size() = " << rhs.nodes_.size()
                      << "\n";
        }
#endif
        return false;
    }

    for (size_t i = 0; i < node_vec_size; ++i) {

        Task* task = nodes_[i]->isTask();
        if (task) {
            Task* rhs_task = rhs.nodes_[i]->isTask();
            if (!rhs_task) {
#ifdef DEBUG
                if (Ecf::debug_equality()) {
                    std::cout << "NodeContainer::operator==  if ( !rhs_task ) " << absNodePath() << "\n";
                }
#endif
                return false;
            }

            if (!(*task == *rhs_task)) {
#ifdef DEBUG
                if (Ecf::debug_equality()) {
                    std::cout << "NodeContainer::operator==  if ( !( *task == *rhs_task )) " << absNodePath() << "\n";
                }
#endif
                return false;
            }
        }
        else {
            Family* rhs_family = rhs.nodes_[i]->isFamily();
            if (!rhs_family) {
#ifdef DEBUG
                if (Ecf::debug_equality()) {
                    std::cout << "NodeContainer::operator==  if ( !rhs_family ) " << absNodePath() << "\n";
                }
#endif
                return false;
            }

            Family* family = nodes_[i]->isFamily();
            LOG_ASSERT(family, "");
            if (family /*keep clang happy*/ && !(*family == *rhs_family)) {
#ifdef DEBUG
                if (Ecf::debug_equality()) {
                    std::cout << "NodeContainer::operator==  if ( !( *family == *rhs_family )) " << absNodePath()
                              << "\n";
                }
#endif
                return false;
            }
        }
    }

    return Node::operator==(rhs);
}

bool NodeContainer::checkInvariants(std::string& errorMsg) const {
    if (!Node::checkInvariants(errorMsg)) {
        return false;
    }

    for (const auto& n : nodes_) {
        if (n->parent() != this) {
            errorMsg += "NodeContainer::checkInvariants family/task parent() not correct";
            return false;
        }
        if (!n->checkInvariants(errorMsg)) {
            return false;
        }
    }
    return true;
}

void NodeContainer::verification(std::string& errorMsg) const {
    Node::verification(errorMsg);
    for (const auto& n : nodes_) {
        n->verification(errorMsg);
    }
}

void NodeContainer::setRepeatToLastValueHierarchically() {
    setRepeatToLastValue();
    for (const auto& n : nodes_) {
        n->setRepeatToLastValueHierarchically();
    }
}

void NodeContainer::setStateOnlyHierarchically(NState::State s, bool force) {
    setStateOnly(s, force);
    for (const auto& n : nodes_) {
        n->setStateOnlyHierarchically(s, force);
    }
}

void NodeContainer::setStateOnlyHierarchically(NState::State s, const Ctx& ctx, bool force) {
    if (!ctx.allows(this->absNodePath(), ecf::Allowed::WRITE)) {
        return;
    }

    setStateOnly(s, force);
    for (const auto& n : nodes_) {
        n->setStateOnlyHierarchically(s, ctx, force);
    }
}

void NodeContainer::set_state_hierarchically(NState::State s, bool force) {
    setStateOnlyHierarchically(s, force);
    if (force) {
        // *force* is only set via ForceCmd.
        update_limits(); // hierarchical
    }
    handleStateChange(); // non-hierarchical
}

void NodeContainer::set_state_hierarchically(NState::State s, const Ctx& ctx, bool force) {
    if (!ctx.allows(this->absNodePath(), ecf::Allowed::WRITE)) {
        return;
    }

    setStateOnlyHierarchically(s, ctx, force);
    if (force) {
        // *force* is only set via ForceCmd.
        update_limits(); // hierarchical
    }
    handleStateChange(); // non-hierarchical
}

void NodeContainer::update_limits() {
    /// Only tasks can affect the limits, hence no point calling locally
    for (const auto& n : nodes_) {
        n->update_limits();
    }
}

std::string NodeContainer::archive_path() const {
    std::string the_archive_path;
    if (!findParentUserVariableValue(ecf::environment::ECF_HOME, the_archive_path)) {
        std::stringstream ss;
        ss << "NodeContainer::archive_path: cannot find ECF_HOME from " << debugNodePath();
        throw std::runtime_error(ss.str());
    }

    std::string the_archive_file_name = absNodePath();
    Str::replaceall(the_archive_file_name, "/", ":"); // we use ':' since it is not allowed in the node names
    the_archive_file_name += ".check";

    std::string port = Str::DEFAULT_PORT_NUMBER();
    Defs* the_defs   = defs();
    if (the_defs) {
        port = the_defs->server_state().find_variable(ecf::environment::ECF_PORT);
        if (port.empty()) {
            port = Str::DEFAULT_PORT_NUMBER();
        }
    }
    Host host;
    the_archive_file_name = host.prefix_host_and_port(port, the_archive_file_name);

    the_archive_path += "/";
    the_archive_path += the_archive_file_name;
    return the_archive_path;
}

void NodeContainer::archive() {
    if (nodes_.empty()) {
        return; // nothing to archive
    }

    SuiteChanged1 changed(suite());

    // make a clone of this node DEEP COPY
    node_ptr this_clone = clone();

    // re-create node tree up to the def. Do *NOT* clone we just need a SHALLOW hierarchy
    defs_ptr archive_defs = Defs::create();
    if (isSuite()) {
        suite_ptr suite_clone = std::dynamic_pointer_cast<Suite>(this_clone);
        archive_defs->addSuite(suite_clone);
    }
    else {
        Node* parent_ptr = parent();
        while (parent_ptr) {
            if (parent_ptr->isSuite()) {
                suite_ptr parent_suite = Suite::create(parent_ptr->name());
                parent_suite->addChild(this_clone);
                archive_defs->addSuite(parent_suite);
                break;
            }
            else {
                family_ptr parent_family = Family::create(parent_ptr->name());
                parent_family->addChild(this_clone);
                this_clone = parent_family;
            }
            parent_ptr = parent_ptr->parent();
        }
    }

    // save the created defs, to disk
    archive_defs->write_to_checkpt_file(archive_path());

    get_flag().set(ecf::Flag::ARCHIVED); // flag as archived
    get_flag().clear(ecf::Flag::RESTORED);

    // delete the child nodes, set parent to null first.
    for (auto& n : nodes_) {
        n->set_parent(nullptr);
    }
    nodes_.clear();

    std::vector<node_ptr>().swap(nodes_);                      // reclaim vector memory
    add_remove_state_change_no_ = Ecf::incr_state_change_no(); // For sync
    string msg                  = " autoarchive ";
    msg += debugNodePath(); // inform user via log
    ecf::log(Log::LOG, msg);
}

void NodeContainer::swap(NodeContainer& rhs) {
    std::swap(nodes_, rhs.nodes_);
    for (auto& n : nodes_) {
        n->set_parent(this);
    }
}

void NodeContainer::restore_on_begin_or_requeue() {
    if (!get_flag().is_set(ecf::Flag::ARCHIVED)) {
        return;
    }
    if (!nodes_.empty()) {
        return;
    }
    if (!fs::exists(archive_path())) {
        return;
    }

    // Node::requeue(...) will clear ecf::Flag::RESTORED, set in restore()
    try {
        restore();
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "NodeContainer::restore_on_begin_or_requeue(): failed : " << e.what();
        log(Log::ERR, ss.str());
    }
}

void NodeContainer::restore() {
    if (!get_flag().is_set(ecf::Flag::ARCHIVED)) {
        std::stringstream ss;
        ss << "NodeContainer::restore() Node " << absNodePath() << " can't restore, ecf::Flag::ARCHIVED not set";
        throw std::runtime_error(ss.str());
    }

    if (!nodes_.empty()) {
        std::stringstream ss;
        ss << "NodeContainer::restore() Node " << absNodePath() << " can't restore, Container already has children ?";
        throw std::runtime_error(ss.str());
    }

    defs_ptr archive_defs        = Defs::create();
    std::string the_archive_path = archive_path();
    try {
        archive_defs->restore(the_archive_path);
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "NodeContainer::restore() Node " << absNodePath() << " could not restore file at  " << the_archive_path
           << "  : " << e.what();
        throw std::runtime_error(ss.str());
    }

    // find the same node in the defs.
    node_ptr archived_node = archive_defs->findAbsNode(absNodePath());
    if (!archived_node) {
        std::stringstream ss;
        ss << "NodeContainer::restore() could not find " << absNodePath() << " in the archived file "
           << the_archive_path;
        throw std::runtime_error(ss.str());
    }
    NodeContainer* archived_node_container = archived_node->isNodeContainer();
    if (!archived_node_container) {
        std::stringstream ss;
        ss << "NodeContainer::restore() The node at " << absNodePath() << " recovered from " << the_archive_path
           << " is not a container(suite/family)";
        throw std::runtime_error(ss.str());
    }

    swap(*archived_node_container);                            // swap the children, and set parent pointers
    get_flag().clear(ecf::Flag::ARCHIVED);                     // clear flag archived
    get_flag().set(ecf::Flag::RESTORED);                       // set restored flag, to stop automatic autoarchive
    add_remove_state_change_no_ = Ecf::incr_state_change_no(); // For sync

    string msg = " autorestore ";
    msg += debugNodePath(); // inform user via log
    ecf::log(Log::LOG, msg);

    fs::remove(the_archive_path); // remove the file, could still throw
}

bool NodeContainer::has_archive() const {
    if (get_flag().is_set(ecf::Flag::ARCHIVED)) {
        return true;
    }
    for (auto& n : nodes_) {
        if (n->has_archive()) {
            return true;
        }
    }
    return false;
}

void NodeContainer::remove_archived_files() {
    /// Called during delete, remove all child archived files
    if (!has_archive()) {
        return;
    }

    std::string ecf_home;
    if (!findParentUserVariableValue(ecf::environment::ECF_HOME, ecf_home)) {
        return;
    }

    // <host>.<port>.ECF_NAME.check
    // for /ecflow =  Avis-MacBook-Pro.local.4040.:ecflow.check
    std::string the_archive_path = archive_path();

    // remove trailing '.check'
    std::string::size_type check_pos = the_archive_path.rfind(".check");
    if (check_pos == std::string::npos) {
        // something has gone wrong
        return;
    }
    the_archive_path.erase(the_archive_path.begin() + check_pos, the_archive_path.end());

    // Find *all* archived files in ECF_HOME
    std::vector<fs::path> archived_file_vec;
    File::find_files_with_extn(ecf_home, ".check", archived_file_vec);
    if (archived_file_vec.empty()) {
        return;
    }

    // if this nodes archive file is a prefix for any archived file, then it is child archived file and needs
    // to be deleted. i.e. if /ecflow is being deleted then:
    // the_archive_path = Avis-MacBook-Pro.local.4040.:ecflow
    // Hence if this matches as a prefix for any archive file, then it needs to be deleted.
    //  Avis-MacBook-Pro.local.4040.:ecflow.check
    //  Avis-MacBook-Pro.local.4040.:ecflow:darwin.check
    //  Avis-MacBook-Pro.local.4040.:ecflow:darwin:apple_clang.check
    //  Avis-MacBook-Pro.local.4040.:ecflow:darwin:gnu.10.check

    for (const auto& file_path : archived_file_vec) {
        std::string path = file_path.string();

        if (path.find(the_archive_path) == 0) {
            try {
                fs::remove(path);
            }
            catch (...) {
            };
        }
    }
}

void NodeContainer::sort_attributes(ecf::Attr::Type attr, bool recursive, const std::vector<std::string>& no_sort) {
    Node::sort_attributes(attr, recursive, no_sort);
    if (recursive) {
        for (const auto& n : nodes_) {
            n->sort_attributes(attr, recursive, no_sort);
        }
    }
}

bool NodeContainer::doDeleteChild(Node* child) {
    SuiteChanged1 changed(suite());
    auto theTaskEnd = nodes_.end();
    for (auto t = nodes_.begin(); t != theTaskEnd; ++t) {
        if ((*t).get() == child) {

            // Remove any child archived file
            Family* fam = (*t)->isFamily();
            if (fam) {
                fam->remove_archived_files();
            }

            child->set_parent(nullptr); // must set to NULL, allow it to be re-added to different parent
            nodes_.erase(t);
            add_remove_state_change_no_ = Ecf::incr_state_change_no();
            set_most_significant_state_up_node_tree();
            return true;
        }
        if ((*t)->doDeleteChild(child)) {
            return true;
        }
    }

    return false;
}

void NodeContainer::check_job_creation(job_creation_ctrl_ptr jobCtrl) {

    if (defStatus() != DState::COMPLETE) {
        for (const auto& n : nodes_) {
            n->check_job_creation(jobCtrl);
        }
    }
}

void NodeContainer::generate_scripts(const std::map<std::string, std::string>& override) const {
    for (const auto& n : nodes_) {
        n->generate_scripts(override);
    }
}

template <class Archive>
void NodeContainer::serialize(Archive& ar, std::uint32_t const version) {
    ar(cereal::base_class<Node>(this), CEREAL_NVP(nodes_));

    // Set up the parent pointers. Since they are not serialised
    if (Archive::is_loading::value) {
        for (auto& n : nodes_) {
            n->set_parent(this);
        }
    }
}
CEREAL_TEMPLATE_SPECIALIZE_V(NodeContainer);
