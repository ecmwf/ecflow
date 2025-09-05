/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Alias.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/core/Str.hpp"

using namespace ecf;
using namespace std;

//==================================================================================
Alias::Alias(const std::string& name, bool check) : Submittable(name, check) {
    set_state_only(NState::QUEUED);
}

Alias::Alias(const Alias& rhs) = default;

Alias::Alias() {
    set_state_only(NState::QUEUED);
}

node_ptr Alias::clone() const {
    return std::make_shared<Alias>(*this);
}

Alias::~Alias() {
    if (!Ecf::server()) {
        notify_delete();
    }
}

Alias& Alias::operator=(const Alias& rhs) {
    if (this != &rhs) {
        Submittable::operator=(rhs);
    }
    return *this;
}

alias_ptr Alias::create(const std::string& name, bool check) {
    return std::make_shared<Alias>(name, check);
}

bool Alias::operator==(const Alias& rhs) const {
    return Submittable::operator==(rhs);
}

void Alias::begin() {
    Submittable::begin();
}

void Alias::requeue(Requeue_args& args) {
    Submittable::requeue(args);
}

const std::string& Alias::debugType() const {
    return ecf::Str::ALIAS();
}

node_ptr Alias::removeChild(Node*) {
    LOG_ASSERT(false, "");
    return node_ptr();
}

bool Alias::addChild(const node_ptr&, size_t) {
    LOG_ASSERT(false, "");
    return false;
}

size_t Alias::child_position(const Node*) const {
    return std::numeric_limits<std::size_t>::max();
}

bool Alias::isAddChildOk(Node* alias, std::string& errorMsg) const {
    errorMsg += "Cannot add children to a Alias";
    return false;
}

void Alias::handleStateChange() {
    Node::handleStateChange();

    /// Increment/decrement limits based on the current state
    update_limits();

    // Aliases are stand alone, they do no requeue or bubble up/down state changes
    // i.e no requeue since they have no time dependencies, or repeat
}

const std::string& Alias::script_extension() const {
    return File::USR_EXTN();
}

void Alias::collateChanges(DefsDelta& changes) const {
    /// All changes to Alias should be on ONE compound_memento_ptr
    compound_memento_ptr comp;
    Submittable::incremental_changes(changes, comp);
}

void Alias::get_all_nodes(std::vector<node_ptr>& nodes) const {
    nodes.push_back(non_const_this());
}

// Functions unique to aliases
void Alias::add_alias_variable(const std::string& name, const std::string& value) {
    if (name.empty()) {
        throw std::runtime_error("Alias::add_alias_variable: Variable with empty name");
    }

    // The bool argument to variable, allows addition of Variable without name checking
    addVariable(Variable(name, value, false));
}

node_ptr Alias::find_node_up_the_tree(const std::string& name) const {
    Node* the_parent = parent();
    if (the_parent) {
        return the_parent->find_node_up_the_tree(name);
    }
    return node_ptr();
}

template <class Archive>
void Alias::serialize(Archive& ar, std::uint32_t const version) {
    ar(cereal::base_class<Submittable>(this));
}
CEREAL_TEMPLATE_SPECIALIZE_V(Alias);
CEREAL_REGISTER_TYPE(Alias)
