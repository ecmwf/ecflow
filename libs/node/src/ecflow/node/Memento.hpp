/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Memento_HPP
#define ecflow_node_Memento_HPP

///
/// \note Classes derived from Memento are stored on DefsDelta.
/// DefsDelta is transferred from Server to Client during a sync. See SSyncCmd.hpp
///
/// Are created in the server, but used by the client to sync.
///
/// Used to capture incremental change of state, to node, and node attributes
/// Serve as a base class of all memento's
/// Later on the client side, the changes can be applied. via incremental_sync()
/// The are several kind of changes that we can capture:
///   a/ simple state changes,
///   b/ Change in attribute structure
///   c/ Deletion of attribute
///   d/ Addition of an attribute
///   e/ Add/delete of a Family/task
///   f/ Add/Delete of suite
///
/// The main emphasis here is to capture a,b,c,d,e. This is easily handled by state_change_no.
/// option f/ is handled via a full update and hence does not use mementos
///
/// ISSUES: AIX has issues with TOC(table of contents) overflow. This is heavily
/// influenced by the number of global symbols. Unfortunately each boost serializable
/// type, greatly increases the number of globals.
/// Hence, we need to ensure we use the minimum number of serializable types.
///

#include "ecflow/attribute/GenericAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

// #define DEBUG_MEMENTO 1

class Memento {
public:
    Memento()               = default;
    Memento(const Memento&) = delete;
    virtual ~Memento();

    Memento& operator=(const Memento&) = delete;

private:
    /// Applies the mementos to the client side defs. Can raise std::runtime_error
    virtual void do_incremental_node_sync(Node*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    virtual void do_incremental_task_sync(Task*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    virtual void do_incremental_alias_sync(Alias*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    virtual void do_incremental_suite_sync(Suite*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    virtual void do_incremental_family_sync(Family*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    virtual void do_incremental_defs_sync(Defs*, std::vector<ecf::Aspect::Type>& aspects, bool) const {}
    friend class CompoundMemento;

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

// Used for storing all the memento's associated with a single node
// This allows to make only *ONE* call to find the node.
// The mementos are then applied to this single node.
/**
 * Represents a set of changes to apply on specific node.
 *
 * The memento captures multiple other mementos, and the path of the node where to apply them on the recipient defs.
 */
class CompoundMemento {
public:
    explicit CompoundMemento(const std::string& absNodePath) : absNodePath_(absNodePath) {}

    CompoundMemento() = default; // for serialization

    void incremental_sync(defs_ptr client_def) const;
    void add(memento_ptr m) { vec_.push_back(m); }
    void clear_attributes() { clear_attributes_ = true; }

    const std::string& abs_node_path() const { return absNodePath_; }

private:
    std::string absNodePath_;
    std::vector<memento_ptr> vec_;
    mutable std::vector<ecf::Aspect::Type> aspects_; // not persisted only used on client side
    bool clear_attributes_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the state of the defs.
 *
 * The memento captures the new state, in order to apply on the recipient defs.
 */
class StateMemento : public Memento {
public:
    explicit StateMemento(NState::State state) : state_(state) {}
    StateMemento() = default;

private:
    void do_incremental_defs_sync(Defs* defs, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        defs->set_memento(this, aspects, f);
    }

    NState::State state_{NState::UNKNOWN};
    friend class Defs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the state of the node.
 *
 * The memento captures the new state and the time it was updated, in order to apply on the recipient node.
 */
class NodeStateMemento : public Memento {
public:
    explicit NodeStateMemento(std::pair<NState::State, boost::posix_time::time_duration> state) : state_(state) {}
    NodeStateMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    std::pair<NState::State, boost::posix_time::time_duration> state_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the order of the immediate children of the node.
 *
 * The memento captures the ordered list of node names, used to reorder the children on the recipient node.
 */
class OrderMemento : public Memento {
public:
    explicit OrderMemento(const std::vector<std::string>& order) : order_(order) {}
    OrderMemento() = default;

private:
    void do_incremental_defs_sync(Defs* defs, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        defs->set_memento(this, aspects, f);
    }
    void do_incremental_suite_sync(Suite* s, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        s->set_memento(this, aspects, f);
    }
    void do_incremental_family_sync(Family* f, std::vector<ecf::Aspect::Type>& aspects, bool ff) const override {
        f->set_memento(this, aspects, ff);
    }
    void do_incremental_task_sync(Task* t, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        t->set_memento(this, aspects, f);
    }

    std::vector<std::string> order_;
    friend class NodeContainer;
    friend class Task;
    friend class Defs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a removal/addition in the immediate children of the node.
 *
 * The memento captures the updated collection of children nodes that must be 'grafted' onto the recipient node.
 */
class ChildrenMemento : public Memento {
public:
    explicit ChildrenMemento(const std::vector<node_ptr>& children) : children_(children) {}
    ChildrenMemento() = default;

private:
    void do_incremental_suite_sync(Suite* s, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        s->set_memento(this, aspects, f);
    }
    void do_incremental_family_sync(Family* f, std::vector<ecf::Aspect::Type>& aspects, bool ff) const override {
        f->set_memento(this, aspects, ff);
    }

    std::vector<node_ptr> children_;
    friend class NodeContainer;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a removal/addition of an alias of a task.
 *
 * The memento captures the updated collection of aliases nodes that must be 'grafted' onto the recipient task.
 */
class AliasChildrenMemento : public Memento {
public:
    explicit AliasChildrenMemento(const std::vector<alias_ptr>& children) : children_(children) {}
    AliasChildrenMemento() = default;

private:
    void do_incremental_task_sync(Task* t, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        t->set_memento(this, aspects, f);
    }

    std::vector<alias_ptr> children_;
    friend class Task;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the alias_no_ of a task.
 *
 * The memento captures the updated alias_no_ to update the recipient task.
 */
class AliasNumberMemento : public Memento {
public:
    explicit AliasNumberMemento(unsigned int alias_no) : alias_no_(alias_no) {}
    AliasNumberMemento() = default;

private:
    void do_incremental_task_sync(Task* t, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        t->set_memento(this, aspects, f);
    }

    unsigned int alias_no_{0};
    friend class Task;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the suspend state of a node.
 *
 * The memento captures the updated suspend state to update the recipient task.
 */
class SuspendedMemento : public Memento {
public:
    explicit SuspendedMemento(bool suspended) : suspended_(suspended) {}
    SuspendedMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    bool suspended_{false};
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a change in the server state.
 *
 * The memento captures the updated server state to update the recipient defs.
 */
class ServerStateMemento : public Memento {
public:
    explicit ServerStateMemento(SState::State s) : state_(s) {}
    ServerStateMemento() = default;

private:
    void do_incremental_defs_sync(Defs* defs, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        defs->set_memento(this, aspects, f);
    }

    SState::State state_{SState::HALTED};
    friend class Defs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents a removal/addition of a variable on the server.
 *
 * The memento captures the updated list of variables to update the recipient defs.
 */
class ServerVariableMemento : public Memento {
public:
    explicit ServerVariableMemento(const std::vector<Variable>& vec) : serverEnv_(vec) {}
    ServerVariableMemento() = default;

private:
    void do_incremental_defs_sync(Defs* defs, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        defs->set_memento(this, aspects, f);
    }

    std::vector<Variable> serverEnv_;
    friend class Defs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to the default state of a node.
 *
 * The memento captures the updated default state to update the recipient node.
 */
class NodeDefStatusDeltaMemento : public Memento {
public:
    explicit NodeDefStatusDeltaMemento(DState::State state) : state_(state) {}
    NodeDefStatusDeltaMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    DState::State state_{DState::UNKNOWN};
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node event.
 *
 * The memento captures the updated event to update the recipient node.
 */
class NodeEventMemento : public Memento {
public:
    explicit NodeEventMemento(const Event& e) : event_(e) {}
    NodeEventMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Event event_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node meter.
 *
 * The memento captures the updated meter to update the recipient node.
 */
class NodeMeterMemento : public Memento {
public:
    explicit NodeMeterMemento(const Meter& e) : meter_(e) {}
    NodeMeterMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Meter meter_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node label.
 *
 * The memento captures the updated label to update the recipient node.
 */
class NodeLabelMemento : public Memento {
public:
    explicit NodeLabelMemento(const Label& e) : label_(e) {}
    NodeLabelMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Label label_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node queue.
 *
 * The memento captures the updated queue to update the recipient node.
 */
class NodeQueueMemento : public Memento {
public:
    explicit NodeQueueMemento(const QueueAttr& e) : queue_(e) {}
    NodeQueueMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    QueueAttr queue_;
    friend class Node;
    friend class MiscAttrs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node generic.
 *
 * The memento captures the updated generic to update the recipient node.
 */
class NodeGenericMemento : public Memento {
public:
    explicit NodeGenericMemento(const GenericAttr& e) : generic_(e) {}
    NodeGenericMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    GenericAttr generic_;
    friend class Node;
    friend class MiscAttrs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node queue state.
 *
 * The memento captures the updated queue state (name, state vector, current index) to update the recipient node.
 */
class NodeQueueIndexMemento : public Memento {
public:
    NodeQueueIndexMemento(const std::string& name, int index, const std::vector<NState::State>& state_vec)
        : name_(name),
          state_vec_(state_vec),
          index_(index) {}
    NodeQueueIndexMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    std::string name_;
    std::vector<NState::State> state_vec_;
    int index_{0};
    friend class Node;
    friend class MiscAttrs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node trigger expression.
 *
 * The memento captures the updated trigger expression to update the recipient node.
 */
class NodeTriggerMemento : public Memento {
public:
    explicit NodeTriggerMemento(const Expression& e) : exp_(e) {}
    NodeTriggerMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Expression exp_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node complete expression.
 *
 * The memento captures the updated complete expression to update the recipient node.
 */
class NodeCompleteMemento : public Memento {
public:
    explicit NodeCompleteMemento(const Expression& e) : exp_(e) {}
    NodeCompleteMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Expression exp_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node repeat.
 *
 * The memento captures the updated repeat to update the recipient node.
 */
class NodeRepeatMemento : public Memento {
public:
    explicit NodeRepeatMemento(const Repeat& e) : repeat_(e) {}
    NodeRepeatMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Repeat repeat_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to the current index of a node repeat.
 *
 * The memento captures the updated repeat index to update the recipient node.
 */
class NodeRepeatIndexMemento : public Memento {
public:
    explicit NodeRepeatIndexMemento(const Repeat& e) : index_or_value_(e.index_or_value()) {}
    NodeRepeatIndexMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    long index_or_value_{0};
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node limit.
 *
 * The memento captures the updated limit to update the recipient node.
 */
class NodeLimitMemento : public Memento {
public:
    explicit NodeLimitMemento(const Limit& e) : limit_(e) {}
    NodeLimitMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Limit limit_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node inlimit.
 *
 * The memento captures the updated inlimit to update the recipient node.
 */
class NodeInLimitMemento : public Memento {
public:
    explicit NodeInLimitMemento(const InLimit& e) : inlimit_(e) {}
    NodeInLimitMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    InLimit inlimit_;
    friend class Node;
    friend class InLimitMgr;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node variable.
 *
 * The memento captures the updated variable to update the recipient node.
 */
class NodeVariableMemento : public Memento {
public:
    explicit NodeVariableMemento(const Variable& e) : var_(e) {}
    NodeVariableMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    Variable var_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node late.
 *
 * The memento captures the updated late to update the recipient node.
 */
class NodeLateMemento : public Memento {
public:
    explicit NodeLateMemento(const ecf::LateAttr& e) : late_(e) {}
    NodeLateMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::LateAttr late_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node or server flag.
 *
 * The memento captures the flag limit to update the recipient node.
 */
class FlagMemento : public Memento {
public:
    explicit FlagMemento(const ecf::Flag& e) : flag_(e) {}
    FlagMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }
    void do_incremental_defs_sync(Defs* defs, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        defs->set_memento(this, aspects, f);
    }

    ecf::Flag flag_;
    friend class Node;
    friend class Defs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node today.
 *
 * The memento captures the updated today to update the recipient node.
 */
class NodeTodayMemento : public Memento {
public:
    explicit NodeTodayMemento(const ecf::TodayAttr& attr) : attr_(attr) {}
    NodeTodayMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::TodayAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node time.
 *
 * The memento captures the updated time to update the recipient node.
 */
class NodeTimeMemento : public Memento {
public:
    explicit NodeTimeMemento(const ecf::TimeAttr& attr) : attr_(attr) {}
    NodeTimeMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::TimeAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node day.
 *
 * The memento captures the updated day to update the recipient node.
 */
class NodeDayMemento : public Memento {
public:
    explicit NodeDayMemento(const DayAttr& attr) : attr_(attr) {}
    NodeDayMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    DayAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node cron.
 *
 * The memento captures the updated cron to update the recipient node.
 */
class NodeCronMemento : public Memento {
public:
    explicit NodeCronMemento(const ecf::CronAttr& attr) : attr_(attr) {}
    NodeCronMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::CronAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node aviso.
 *
 * The memento captures the updated aviso to update the recipient node.
 */
class NodeAvisoMemento : public Memento {
public:
    NodeAvisoMemento() = default;
    explicit NodeAvisoMemento(const ecf::AvisoAttr& a) : aviso_(a.make_detached()) {}

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::AvisoAttr aviso_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node mirror.
 *
 * The memento captures the updated mirror to update the recipient node.
 */
class NodeMirrorMemento : public Memento {
public:
    NodeMirrorMemento() = default;
    explicit NodeMirrorMemento(const ecf::MirrorAttr& a) : mirror_(a.make_detached()) {}

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::MirrorAttr mirror_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node date.
 *
 * The memento captures the updated date to update the recipient node.
 */
class NodeDateMemento : public Memento {
public:
    explicit NodeDateMemento(const DateAttr& attr) : attr_(attr) {}
    NodeDateMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    DateAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node zombie.
 *
 * The memento captures the updated zombie to update the recipient node.
 */
class NodeZombieMemento : public Memento {
public:
    explicit NodeZombieMemento(const ZombieAttr& attr) : attr_(attr) {}
    NodeZombieMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ZombieAttr attr_;
    friend class Node;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a node verify - n.b. VerifyAttr is used for internal debugging purposes only.
 *
 * The memento captures the updated verify to update the recipient node.
 */
class NodeVerifyMemento : public Memento {
public:
    explicit NodeVerifyMemento(const std::vector<VerifyAttr>& attr) : verifys_(attr) {}
    NodeVerifyMemento() = default;

private:
    void do_incremental_node_sync(Node* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    std::vector<VerifyAttr> verifys_;
    friend class Node;
    friend class MiscAttrs;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to a submittable node (i.e. Task or Alias).
 *
 * The memento captures the job password, rid, try no, and reason for early termination to update the recipient node.
 */
class SubmittableMemento : public Memento {
public:
    SubmittableMemento(const std::string& jobsPassword,
                       const std::string& process_or_remote_id,
                       const std::string& abortedReason,
                       int tryNo)
        : paswd_(jobsPassword),
          rid_(process_or_remote_id),
          abr_(abortedReason),
          tryNo_(tryNo) {}
    SubmittableMemento() = default;

private:
    void do_incremental_task_sync(Task* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }
    void do_incremental_alias_sync(Alias* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    std::string paswd_;
    std::string rid_;
    std::string abr_;
    int tryNo_{0};
    friend class Submittable;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to the suite's clock.
 *
 * The memento captures the updated clock to update the recipient suite.
 */
class SuiteClockMemento : public Memento {
public:
    explicit SuiteClockMemento(const ClockAttr& c) : clockAttr_(c) {}
    SuiteClockMemento() = default;

private:
    void do_incremental_suite_sync(Suite* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ClockAttr clockAttr_;
    friend class Suite;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to the suite's begin status.
 *
 * The memento captures the begin status to update the recipient suite.
 */
class SuiteBeginDeltaMemento : public Memento {
public:
    explicit SuiteBeginDeltaMemento(bool begun) : begun_(begun) {}
    SuiteBeginDeltaMemento() = default;

private:
    void do_incremental_suite_sync(Suite* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    bool begun_{false};
    friend class Suite;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

/**
 * Represents an update to the suite's calendar.
 *
 * The memento captures the updated calendar to update the recipient suite.
 */
class SuiteCalendarMemento : public Memento {
public:
    explicit SuiteCalendarMemento(const ecf::Calendar& cal) : cal_(cal) {}
    SuiteCalendarMemento() = default;

private:
    void do_incremental_suite_sync(Suite* n, std::vector<ecf::Aspect::Type>& aspects, bool f) const override {
        n->set_memento(this, aspects, f);
    }

    ecf::Calendar cal_; // *Only* persisted since used by the why() on client side
    friend class Suite;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

#endif /* ecflow_node_Memento_HPP */
