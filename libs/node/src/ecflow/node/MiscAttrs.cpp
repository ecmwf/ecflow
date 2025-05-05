/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/MiscAttrs.hpp"

#include <stdexcept>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/Node.hpp"

#ifdef DEBUG
    #include <iostream>
#endif

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

MiscAttrs::MiscAttrs(const MiscAttrs& rhs)
    : zombies_(rhs.zombies_),
      verifys_(rhs.verifys_),
      queues_(rhs.queues_),
      generics_(rhs.generics_) {
}

MiscAttrs::~MiscAttrs() = default;

// needed by node serialisation
void MiscAttrs::set_node(Node* n) {
    node_ = n;
}

bool MiscAttrs::checkInvariants(std::string& errorMsg) const {
    if (!node_) {
        errorMsg += "MiscAttrs::checkInvariants: failed node_ is NULL";
        return false;
    }
    return true;
}

void MiscAttrs::begin() {
    // reset verification
    for (auto& verify : verifys_) {
        verify.reset();
    }
    for (auto& queue : queues_) {
        queue.requeue();
    }
}

void MiscAttrs::requeue() {
    for (auto& queue : queues_) {
        queue.requeue();
    }
}

void MiscAttrs::print(std::string& os) const {
    for (const ZombieAttr& z : zombies_) {
        z.print(os);
    }
    for (const VerifyAttr& v : verifys_) {
        v.print(os);
    }
    for (const QueueAttr& q : queues_) {
        q.print(os);
    }
    for (const GenericAttr& g : generics_) {
        g.print(os);
    }
}

bool MiscAttrs::operator==(const MiscAttrs& rhs) const {
    if (zombies_.size() != rhs.zombies_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==   (zombies_.size() != rhs.zombies_.size()) " << node_->debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < zombies_.size(); ++i) {
        if (!(zombies_[i] == rhs.zombies_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "MiscAttrs::operator==   (!(zombies_[i] == rhs.zombies_[i]) " << node_->debugNodePath()
                          << "\n";
            }
#endif
            return false;
        }
    }

    if (queues_.size() != rhs.queues_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==   (queues_.size() != rhs.queues_.size()) " << node_->debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < queues_.size(); ++i) {
        if (!(queues_[i] == rhs.queues_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "MiscAttrs::operator==   (!(queues_[i] == rhs.queues_[i]) " << node_->debugNodePath()
                          << "\n";
            }
#endif
            return false;
        }
    }

    if (verifys_.size() != rhs.verifys_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==  (verifys_.size() != rhs.verifys_.size()) " << node_->debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < verifys_.size(); ++i) {
        if (!(verifys_[i] == rhs.verifys_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "MiscAttrs::operator==  (!(verifys_[i] == rhs.verifys_[i] ))  " << node_->debugNodePath()
                          << "\n";
            }
#endif
            return false;
        }
    }

    if (generics_.size() != rhs.generics_.size()) {
#ifdef DEBUG
        if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==  (generics_.size() != rhs.generics_.size()) " << node_->debugNodePath()
                      << "\n";
        }
#endif
        return false;
    }
    for (unsigned i = 0; i < generics_.size(); ++i) {
        if (!(generics_[i] == rhs.generics_[i])) {
#ifdef DEBUG
            if (Ecf::debug_equality()) {
                std::cout << "MiscAttrs::operator==  (!(generics_ [i] == rhs.generics_[i] ))  "
                          << node_->debugNodePath() << "\n";
            }
#endif
            return false;
        }
    }
    return true;
}

void MiscAttrs::verification(std::string& errorMsg) const {
    for (const VerifyAttr& v : verifys_) {
        if (v.expected() != v.actual()) {
            std::stringstream ss;
            ss << node_->debugNodePath() << " expected " << v.expected() << " " << NState::toString(v.state())
               << " but found " << v.actual() << "\n";
            errorMsg += ss.str();
        }
    }
}

void MiscAttrs::addZombie(const ZombieAttr& z) {
    const ZombieAttr& theFndOne = findZombie(z.zombie_type());
    if (!theFndOne.empty()) {
        std::stringstream ss;
        ss << "MiscAttrs::addZombie: Node " << node_->absNodePath() << " already has a zombie attribute of type "
           << Child::to_string(theFndOne.zombie_type()) << "\n";
        throw std::runtime_error(ss.str());
    }
    zombies_.push_back(z);
    node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "MiscAttrs::addZombie()\n";
#endif
}

void MiscAttrs::deleteZombie(const std::string& zombie_type) {
    if (zombie_type.empty()) {
        zombies_.clear();
        node_->state_change_no_ = Ecf::incr_state_change_no();
        return;
    }

    if (!Child::valid_zombie_type(zombie_type)) {
        throw std::runtime_error(
            "MiscAttrs::deleteZombie failed: Expected one of [ ecf | path | user ] or empty string but found " +
            zombie_type);
    }
    delete_zombie(Child::zombie_type(zombie_type));
}

void MiscAttrs::delete_zombie(Child::ZombieType zt) {
    for (size_t i = 0; i < zombies_.size(); ++i) {
        if (zombies_[i].zombie_type() == zt) {
            zombies_.erase(zombies_.begin() + i);
            node_->state_change_no_ = Ecf::incr_state_change_no();
            return;
        }
    }
}

const ZombieAttr& MiscAttrs::findZombie(ecf::Child::ZombieType zombie_type) const {
    /// There should only be one of each type
    for (const auto& zombie : zombies_) {
        if (zombie.zombie_type() == zombie_type) {
            return zombie;
        }
    }
    return ZombieAttr::EMPTY();
}

void MiscAttrs::add_generic(const GenericAttr& z) {
    const GenericAttr& theFndOne = find_generic(z.name());
    if (!theFndOne.empty()) {
        std::stringstream ss;
        ss << "MiscAttrs::add_generic : Node " << node_->absNodePath() << " already has a generic attribute of name "
           << z.name() << "\n";
        throw std::runtime_error(ss.str());
    }
    generics_.push_back(z);
    node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void MiscAttrs::addVerify(const VerifyAttr& v) {
    if (findVerify(v)) {
        std::stringstream ss;
        ss << "Add Verify failed: Duplicate '" << v.toString() << "' already exists for node "
           << node_->debugNodePath();
        throw std::runtime_error(ss.str());
    }
    verifys_.push_back(v);
    node_->state_change_no_ = Ecf::incr_state_change_no();
}

bool MiscAttrs::findVerify(const VerifyAttr& v) const {
    size_t theSize = verifys_.size();
    for (size_t i = 0; i < theSize; i++) {
        if (verifys_[i].state() == v.state()) {
            return true;
        }
    }
    return false;
}

void MiscAttrs::add_queue(const QueueAttr& q) {
    const QueueAttr& theFndOne = find_queue(q.name());
    if (!theFndOne.empty()) {
        std::stringstream ss;
        ss << "MiscAttrs::add_queue: Node " << node_->absNodePath() << " already has a queue attribute of name "
           << q.name() << "\n";
        throw std::runtime_error(ss.str());
    }
    queues_.push_back(q);
    node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void MiscAttrs::delete_queue(const std::string& name) {
    if (name.empty()) {
        queues_.clear();
        node_->state_change_no_ = Ecf::incr_state_change_no();
        return;
    }
    for (size_t i = 0; i < queues_.size(); ++i) {
        if (queues_[i].name() == name) {
            queues_.erase(queues_.begin() + i);
            node_->state_change_no_ = Ecf::incr_state_change_no();
            return;
        }
    }
}

const QueueAttr& MiscAttrs::find_queue(const std::string& name) const {
    for (const auto& queue : queues_) {
        if (queue.name() == name) {
            return queue;
        }
    }
    return QueueAttr::EMPTY();
}

QueueAttr& MiscAttrs::findQueue(const std::string& name) {
    for (auto& queue : queues_) {
        if (queue.name() == name) {
            return queue;
        }
    }
    return QueueAttr::EMPTY1();
}

void MiscAttrs::delete_generic(const std::string& name) {
    if (name.empty()) {
        generics_.clear();
        node_->state_change_no_ = Ecf::incr_state_change_no();
        return;
    }
    for (size_t i = 0; i < generics_.size(); ++i) {
        if (generics_[i].name() == name) {
            generics_.erase(generics_.begin() + i);
            node_->state_change_no_ = Ecf::incr_state_change_no();
            return;
        }
    }
}

const GenericAttr& MiscAttrs::find_generic(const std::string& name) const {
    for (const auto& generic : generics_) {
        if (generic.name() == name) {
            return generic;
        }
    }
    return GenericAttr::EMPTY();
}

void MiscAttrs::set_memento(const NodeQueueMemento* m) {
    add_queue(m->queue_);
}

void MiscAttrs::set_memento(const NodeGenericMemento* m) {
    add_generic(m->generic_);
}

void MiscAttrs::set_memento(const NodeQueueIndexMemento* m) {
    for (auto& queue : queues_) {
        if (queue.name() == m->name_) {
            queue.set_index(m->index_);
            queue.set_state_vec(m->state_vec_);
        }
    }
}

template <class Archive>
void MiscAttrs::serialize(Archive& ar, std::uint32_t const version) {
    CEREAL_OPTIONAL_NVP(ar, zombies_, [this]() { return !zombies_.empty(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, verifys_, [this]() { return !verifys_.empty(); });   // conditionally save
    CEREAL_OPTIONAL_NVP(ar, queues_, [this]() { return !queues_.empty(); });     // conditionally save
    CEREAL_OPTIONAL_NVP(ar, generics_, [this]() { return !generics_.empty(); }); // conditionally save
}
CEREAL_TEMPLATE_SPECIALIZE_V(MiscAttrs);
