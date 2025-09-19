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

    if constexpr (std::is_const_v<N>) {
        // Handle: const N&
        if (auto container = dynamic_cast<const NodeContainer*>(&node); container) {
            // Handle: Suite, Family
            if (selector(*container)) {
                collector(*container);
            }

            // Process children: Family, Task
            for (auto& child : container->children()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto task = dynamic_cast<const Task*>(&node); task) {
            // Handle: Task
            if (selector(*task)) {
                collector(*task);
            }

            // Process children: Alias
            for (auto& child : task->aliases()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto alias = dynamic_cast<const Alias*>(&node); alias) {
            // Handle: Alias
            if (selector(*alias)) {
                collector(*alias);
            }
        }
        else {
            std::ostringstream ss;
            ss << "NodeAlgorithms::select: unexpected node type: " << typeid(node).name();
            throw std::runtime_error(ss.str());
        }
    }
    else {
        // Handle: (non-const) N&
        if (auto container = dynamic_cast<NodeContainer*>(&node); container) {
            // Handle: Suite, Family
            if (selector(*container)) {
                collector(*container);
            }

            // Process children: Family, Task
            for (auto& child : container->children()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto task = dynamic_cast<Task*>(&node); task) {
            // Handle: Task
            if (selector(*task)) {
                collector(*task);
            }

            // Process children: Alias
            for (auto& child : task->aliases()) {
                select_nodes_from_node(*child, collector, selector);
            }
        }
        else if (auto alias = dynamic_cast<Alias*>(&node); alias) {
            // Handle: Alias
            if (selector(*alias)) {
                collector(*alias);
            }
        }
        else {
            std::ostringstream ss;
            ss << "NodeAlgorithms::select: unexpected node type: " << typeid(node).name();
            throw std::runtime_error(ss.str());
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

} // namespace ecf
