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
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAstVisitor.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Task.hpp"

#ifndef ecflow_node_NodeAlgorithms_HPP
    #define ecflow_node_NodeAlgorithms_HPP

namespace ecf {

/**
 * Retrieve the set of (regular) variables of the given node.
 *
 * @param node The node being queried
 * @return The set of variables
 */
inline const std::vector<Variable>& variables(const Node& node) {
    return node.variables();
}

/**
 * Retrieve the set of inherited variables of the given node.
 *
 * @param node The node being queried
 * @return The set of variables
 */
std::vector<Variable> inherited_variables(const Node& node);

/**
 * Retrieve the set of generated (including inherited) variables of the given node.
 *
 * @param node The node being queried
 * @return The set of variables
 */
std::vector<Variable> generated_variables(const Node& node);

// Select Nodes

/**
 * Retrieve all 'sub-nodes' of the given defs.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Node*> get_all_nodes(Defs& defs);
std::vector<const Node*> get_all_nodes(const Defs& defs);

std::vector<node_ptr> get_all_nodes_ptr(Defs& defs);
std::vector<node_ptr> get_all_nodes_ptr(node_ptr& node);

/**
 * Retrieve all 'sub-nodes' of the given node (including the node itself).
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Node*> get_all_nodes(Node& node);

// Select Tasks

/**
 * Retrieve all 'sub-nodes' of the given defs, that are of type Task.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Task*> get_all_tasks(Defs& defs);
std::vector<const Task*> get_all_tasks(const Defs& defs);

std::vector<task_ptr> get_all_tasks_ptr(Defs& defs);

/**
 * Retrieve all 'sub-nodes' of the given node (including the node itself), that are of type Task.
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Task*> get_all_tasks(Node& node);
std::vector<const Task*> get_all_tasks(const Node& node);

// Select Alias

/**
 * Retrieve all 'sub-nodes' of the given defs, that are of type Alias.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Alias*> get_all_aliases(Defs& defs);

/**
 * Retrieve all 'sub-nodes' of the given node (including the node itself), that are of type Alias.
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::vector<const Alias*> get_all_aliases(const Node& node);

// Select Active Submittables

/**
 * Retrieve all 'sub-nodes' of the given defs, that are both Active and of type Submittable.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Submittable*> get_all_active_submittables(Defs& defs);

/**
 * Retrieve all 'sub-nodes' of the given node (including the node itself), that are both Active and of type Submittable.
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Submittable*> get_all_active_submittables(Node& node);

// Select Families

/**
 * Retrieve all 'sub-nodes' of the given defs, that are of type Family.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Family*> get_all_families(const Defs& defs);

/**
 * Retrieve all 'sub-nodes' of the given node (including the node itself), that are of type Family.
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::vector<Family*> get_all_families(Node& node);

// Select Nodes that have ASTs (i.e. either Trigger or Complete)

/**
 * Retrieve the set of 'sub-nodes' of the given defs,
 * that are referenced in either a Trigger or a Complete.
 *
 * @param defs The defs being queried
 * @return The set of 'sub-nodes'
 */
std::set<const Node*> get_all_ast_nodes(const Defs& defs);

/**
 * Retrieve the set of 'sub-nodes' of the given node (including the node itself),
 * that are referenced in either a Trigger or a Complete.
 *
 * @param node The node being queried
 * @return The set of 'sub-nodes'
 */
std::set<const Node*> get_all_ast_nodes(const Node& node);

/**
 * Ensure that all mirrors in the given defs or node are valid,
 *
 * @param defs The defs being checked
 * @param host The host of the server
 * @param port The port of the server
 *
 * @throws std::runtime_error if an invalid mirror is found
 */
void ensure_all_mirrors_are_valid(const Defs& defs, std::string_view host, std::string_view port);

/**
 * Ensure that all mirrors in the given defs or node are valid,
 *
 * @param node The node being checked
 * @param host The host of the server
 * @param port The port of the server
 *
 * @throws std::runtime_error if an invalid mirror is found
 */
void ensure_all_mirrors_are_valid(const Node& node, std::string_view host, std::string_view port);

} // namespace ecf

#endif /* ecflow_node_NodeAlgorithms_HPP */
