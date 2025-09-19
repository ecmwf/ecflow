/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/NodeAlgorithms.hpp"

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

namespace implementation {

template <typename N, typename Selector, typename Collector>
void select_nodes_from_node(N& node, Collector& collector, Selector selector) {

    if (selector(node)) {
        collector(node);
    }

    if constexpr (std::is_const_v<N>) {
        // Handle: const N&
        if (auto container = dynamic_cast<const NodeContainer*>(&node); container) {
            // Process children: Family, Task
            for (auto& child : container->children()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto task = dynamic_cast<const Task*>(&node); task) {
            // Process children: Alias
            for (auto& child : task->aliases()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
    }
    else {
        // Handle: (non-const) N&
        if (auto container = dynamic_cast<NodeContainer*>(&node); container) {
            // Process children: Family, Task
            for (auto& child : container->children()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto task = dynamic_cast<Task*>(&node); task) {
            // Process children: Alias
            for (auto& child : task->aliases()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
    }
}

template <typename Function, typename Collection>
void select_nodes_from_defs(Defs& defs, Collection& selected, Function selector) {

    // Handle: Defs (non-const)
    for (const auto& s : defs.suites()) {
        select_nodes_from_node(*s, selected, selector);
    }
}

template <typename Function, typename Collection>
void select_nodes_from_defs(const Defs& defs, Collection& selected, Function selector) {

    // Handle: Defs (const)
    for (const auto& s : defs.suites()) {
        select_nodes_from_node(*s, selected, selector);
    }
}

} // namespace implementation

std::vector<Node*> get_all_nodes(Defs& defs) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return true; };
    implementation::select_nodes_from_defs(defs, collector, selector);

    return selected;
}

std::vector<const Node*> get_all_nodes(const Defs& defs) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<const Node*> selected;
    auto collector = [&selected](const Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return true; };
    implementation::select_nodes_from_defs(defs, collector, selector);

    return selected;
}

std::vector<Node*> get_all_nodes(Node& node) {
    // Select all Suite, Family, Tasks and Aliases
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return true; };
    implementation::select_nodes_from_node(node, collector, selector);

    return selected;
}

std::vector<Task*> get_all_tasks(Defs& defs) {
    // Select all Tasks
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isTask(); };
    implementation::select_nodes_from_defs(defs, collector, selector);

    // Downcast to return type
    std::vector<Task*> tasks;
    for (auto& task : selected) {
        if (auto t = dynamic_cast<Task*>(task)) {
            tasks.push_back(t);
        }
    }

    return tasks;
}

std::vector<const Task*> get_all_tasks(const Defs& defs) {
    // Select all Tasks
    std::vector<const Node*> selected;
    auto collector = [&selected](const Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isTask(); };
    implementation::select_nodes_from_defs(defs, collector, selector);

    // Downcast to return type
    std::vector<const Task*> tasks;
    for (auto& task : selected) {
        if (auto t = dynamic_cast<const Task*>(task)) {
            tasks.push_back(t);
        }
    }

    return tasks;
}

std::vector<Task*> get_all_tasks(Node& node) {
    // Select all Tasks
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isTask(); };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<Task*> tasks;
    for (auto& task : selected) {
        if (auto t = dynamic_cast<Task*>(task)) {
            tasks.push_back(t);
        }
    }

    return tasks;
}

std::vector<const Task*> get_all_tasks(const Node& node) {
    // Select all Tasks
    std::vector<const Node*> selected;
    auto collector = [&selected](const Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isTask(); };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<const Task*> tasks;
    for (auto& task : selected) {
        if (auto t = dynamic_cast<const Task*>(task)) {
            tasks.push_back(t);
        }
    }

    return tasks;
}

std::vector<Alias*> get_all_aliases(Defs& defs) {
    // Select all Aliases
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isAlias(); };
    implementation::select_nodes_from_defs(defs, collector, selector);

    // Downcast to return type
    std::vector<Alias*> aliases;
    for (auto& alias : selected) {
        if (auto t = dynamic_cast<Alias*>(alias)) {
            aliases.push_back(t);
        }
    }

    return aliases;
}

std::vector<Alias*> get_all_aliases(Node& node) {
    // Select all Aliases
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isAlias(); };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<Alias*> aliases;
    for (auto& alias : selected) {
        if (auto t = dynamic_cast<Alias*>(alias)) {
            aliases.push_back(t);
        }
    }

    return aliases;
}
std::vector<const Alias*> get_all_aliases(const Node& node) {
    // Select all Aliases
    std::vector<const Node*> selected;
    auto collector = [&selected](const Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isAlias(); };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<const Alias*> aliases;
    for (auto& alias : selected) {
        if (auto t = dynamic_cast<const Alias*>(alias)) {
            aliases.push_back(t);
        }
    }

    return aliases;
}

std::vector<Submittable*> get_all_active_submittables(Defs& defs) {
    // Select all Active Submittables
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) {
        return node.isSubmittable() && (node.state() == NState::ACTIVE || node.state() == NState::SUBMITTED);
    };
    implementation::select_nodes_from_defs(defs, collector, selector);

    // Downcast to return type
    std::vector<Submittable*> submittables;
    for (auto& submittable : selected) {
        if (auto t = submittable->isSubmittable()) {
            submittables.push_back(t);
        }
    }

    return submittables;
}

std::vector<Submittable*> get_all_active_submittables(Node& node) {
    // Select all Active Submittables
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) {
        return node.isSubmittable() && (node.state() == NState::ACTIVE || node.state() == NState::SUBMITTED);
    };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<Submittable*> submittables;
    for (auto& submittable : selected) {
        if (auto t = submittable->isSubmittable()) {
            submittables.push_back(t);
        }
    }

    return submittables;
}

std::vector<Family*> get_all_families(const Defs& defs) {
    // Select all Families
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isFamily(); };
    implementation::select_nodes_from_defs(defs, collector, selector);

    // Downcast to return type
    std::vector<Family*> families;
    for (auto& family : selected) {
        if (auto t = dynamic_cast<Family*>(family)) {
            families.push_back(t);
        }
    }

    return families;
}

std::vector<Family*> get_all_families(Node& node) {
    // Select all Families
    std::vector<Node*> selected;
    auto collector = [&selected](Node& node) { selected.push_back(&node); };
    auto selector  = [](const Node& node) { return node.isFamily(); };
    implementation::select_nodes_from_node(node, collector, selector);

    // Downcast to return type
    std::vector<Family*> families;
    for (auto& family : selected) {
        if (auto t = dynamic_cast<Family*>(family)) {
            families.push_back(t);
        }
    }

    return families;
}

} // namespace ecf
