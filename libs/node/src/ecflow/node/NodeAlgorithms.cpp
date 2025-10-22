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

namespace ecf {

namespace implementation {

template <typename N, typename Visitor>
void select_nodes_from_node(N& node, Visitor& visitor) {

    visitor(node);

    if (auto container = dynamic_cast<const NodeContainer*>(&node); container) {
        // Process children: Family, Task
        for (auto& child : container->children()) {
            select_nodes_from_node(*child, visitor);
        }
    }
    else if (auto task = dynamic_cast<const Task*>(&node); task) {
        // Process children: Alias
        for (auto& child : task->aliases()) {
            select_nodes_from_node(*child, visitor);
        }
    }
}

template <typename Visitor>
void select_nodes_from_defs(Defs& defs, Visitor& visitor) {

    // Handle: Defs (non-const)
    for (const auto& s : defs.suites()) {
        select_nodes_from_node(*s, visitor);
    }
}

template <typename Visitor>
void select_nodes_from_defs(const Defs& defs, Visitor& visitor) {

    // Handle: Defs (const)
    for (const auto& s : defs.suites()) {
        select_nodes_from_node(*s, visitor);
    }
}

template <typename Derived, typename Base>
void downcast_to(const std::vector<Base*>& selected, std::vector<Derived*>& downcasts) {
    for (auto& base : selected) {
        if (auto derived = dynamic_cast<Derived*>(base)) {
            downcasts.push_back(derived);
        }
    }
}

} // namespace implementation

std::vector<Node*> get_all_nodes(Defs& defs) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) { selected.push_back(&node); };
    implementation::select_nodes_from_defs(defs, visitor);

    return selected;
}

std::vector<const Node*> get_all_nodes(const Defs& defs) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<const Node*> selected;
    auto visitor = [&selected](const Node& node) { selected.push_back(&node); };
    implementation::select_nodes_from_defs(defs, visitor);

    return selected;
}

std::vector<Node*> get_all_nodes(Node& node) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) { selected.push_back(&node); };
    implementation::select_nodes_from_node(node, visitor);

    return selected;
}

std::vector<Task*> get_all_tasks(Defs& defs) {
    // Select all Tasks
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isTask()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<Task*> tasks;
    implementation::downcast_to<Task>(selected, tasks);
    return tasks;
}

std::vector<const Task*> get_all_tasks(const Defs& defs) {
    // Select all Tasks
    std::vector<const Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isTask()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<const Task*> tasks;
    implementation::downcast_to<const Task>(selected, tasks);
    return tasks;
}

std::vector<Task*> get_all_tasks(Node& node) {
    // Select all Tasks
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isTask()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Downcast to return type
    std::vector<Task*> tasks;
    implementation::downcast_to<Task>(selected, tasks);
    return tasks;
}

std::vector<const Task*> get_all_tasks(const Node& node) {
    // Select all Tasks
    std::vector<const Node*> selected;
    auto visitor = [&selected](const Node& node) {
        if (node.isTask()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Downcast to return type
    std::vector<const Task*> tasks;
    implementation::downcast_to<const Task>(selected, tasks);
    return tasks;
}

std::vector<Alias*> get_all_aliases(Defs& defs) {
    // Select all Aliases
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isAlias()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<Alias*> aliases;
    implementation::downcast_to<Alias>(selected, aliases);
    return aliases;
}

std::vector<const Alias*> get_all_aliases(const Node& node) {
    // Select all Aliases
    std::vector<const Node*> selected;
    auto visitor = [&selected](const Node& node) {
        if (node.isAlias()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Downcast to return type
    std::vector<const Alias*> aliases;
    implementation::downcast_to<const Alias>(selected, aliases);
    return aliases;
}

std::vector<Submittable*> get_all_active_submittables(Defs& defs) {
    // Select all Active Submittables
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isSubmittable() && (node.state() == NState::ACTIVE || node.state() == NState::SUBMITTED)) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<Submittable*> submittables;
    implementation::downcast_to<Submittable>(selected, submittables);
    return submittables;
}

std::vector<Submittable*> get_all_active_submittables(Node& node) {
    // Select all Active Submittables
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isSubmittable() && (node.state() == NState::ACTIVE || node.state() == NState::SUBMITTED)) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Downcast to return type
    std::vector<Submittable*> submittables;
    implementation::downcast_to<Submittable>(selected, submittables);
    return submittables;
}

std::vector<Family*> get_all_families(const Defs& defs) {
    // Select all Families
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isFamily()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<Family*> families;
    implementation::downcast_to<Family>(selected, families);
    return families;
}

std::vector<Family*> get_all_families(Node& node) {
    // Select all Families
    std::vector<Node*> selected;
    auto visitor = [&selected](Node& node) {
        if (node.isFamily()) {
            selected.push_back(&node);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Downcast to return type
    std::vector<Family*> families;
    implementation::downcast_to<Family>(selected, families);
    return families;
}

std::set<const Node*> get_all_ast_nodes(const Defs& defs) {
    // Select all Nodes with ASTs
    std::set<Node*> selected;
    auto visitor = [&selected](const Node& node) {
        if (node.completeAst()) {
            AstCollateNodesVisitor astVisitor(selected);
            node.completeAst()->accept(astVisitor);
        }
        if (node.triggerAst()) {
            AstCollateNodesVisitor astVisitor(selected);
            node.triggerAst()->accept(astVisitor);
        }
    };
    implementation::select_nodes_from_defs(defs, visitor);

    // Convert to const
    std::set<const Node*> nodes;
    for (auto& node : selected) {
        nodes.insert(node);
    }

    return nodes;
}

std::set<const Node*> get_all_ast_nodes(const Node& node) {
    // Select all Nodes with ASTs
    std::set<Node*> selected;
    auto visitor = [&selected](const Node& node) {
        if (node.completeAst()) {
            AstCollateNodesVisitor astVisitor(selected);
            node.completeAst()->accept(astVisitor);
        }
        if (node.triggerAst()) {
            AstCollateNodesVisitor astVisitor(selected);
            node.triggerAst()->accept(astVisitor);
        }
    };
    implementation::select_nodes_from_node(node, visitor);

    // Convert to const
    std::set<const Node*> nodes;
    for (auto& node : selected) {
        nodes.insert(node);
    }

    return nodes;
}

namespace implementation {

template <typename Visitor>
void select_nodes_ptr_from_node(const node_ptr& node, Visitor& visitor) {

    visitor(node);

    // Handle: (non-const) N&
    if (auto container = std::dynamic_pointer_cast<NodeContainer>(node); container) {
        // Process children: Family, Task
        for (auto& child : container->children()) {
            select_nodes_ptr_from_node(child, visitor);
        }
    }
    else if (auto task = std::dynamic_pointer_cast<Task>(node); task) {
        // Process children: Alias
        for (auto& child : task->aliases()) {
            select_nodes_ptr_from_node(child, visitor);
        }
    }
}

template <typename Visitor>
void select_nodes_ptr_from_defs(Defs& defs, Visitor& visitor) {

    // Handle: Defs (non-const)
    for (const auto& s : defs.suites()) {
        select_nodes_from_node(s, visitor);
    }
}

template <typename Visitor>
void select_nodes_ptr_from_defs(const Defs& defs, Visitor& visitor) {

    // Handle: Defs (const)
    for (const auto& s : defs.suites()) {
        select_nodes_ptr_from_node(s, visitor);
    }
}

template <typename Derived, typename Base>
void downcast_to(const std::vector<std::shared_ptr<Base>>& selected, std::vector<std::shared_ptr<Derived>>& downcasts) {
    for (auto& base : selected) {
        if (auto derived = std::dynamic_pointer_cast<Derived>(base)) {
            downcasts.push_back(derived);
        }
    }
}

template <typename T, typename Filter>
std::vector<std::shared_ptr<T>> get_all_t_ptr(const node_ptr& node, Filter filter) {
    // Select all Nodes, according to filter
    std::vector<node_ptr> selected;
    auto visitor = [&selected, filter](const node_ptr& node) {
        if (filter(node)) {
            selected.push_back(node);
        }
    };
    implementation::select_nodes_ptr_from_node(node, visitor);

    // Downcast to return type
    std::vector<std::shared_ptr<T>> tasks;
    downcast_to<T>(selected, tasks);
    return tasks;
}

template <typename T, typename Filter>
std::vector<std::shared_ptr<T>> get_all_t_ptr(const Defs& defs, Filter filter) {
    // Select all Nodes, according to filter
    std::vector<node_ptr> selected;
    auto visitor = [&selected, filter](const node_ptr& node) {
        if (filter(node)) {
            selected.push_back(node);
        }
    };
    implementation::select_nodes_ptr_from_defs(defs, visitor);

    // Downcast to return type
    std::vector<std::shared_ptr<T>> tasks;
    downcast_to<T>(selected, tasks);
    return tasks;
}

} // namespace implementation

std::vector<task_ptr> get_all_tasks_ptr(Defs& defs) {
    return implementation::get_all_t_ptr<Task>(defs, [](const node_ptr& node) { return node->isTask() != nullptr; });
}

std::vector<node_ptr> get_all_nodes_ptr(Defs& defs) {
    return implementation::get_all_t_ptr<Node>(defs, [](const node_ptr&) { return true; });
}

std::vector<node_ptr> get_all_nodes_ptr(node_ptr& node) {
    return implementation::get_all_t_ptr<Node>(node, [](const node_ptr&) { return true; });
}

} // namespace ecf
