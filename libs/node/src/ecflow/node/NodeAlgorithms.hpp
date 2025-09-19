/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_NodeAlgorithms_hpp
#define ecflow_node_NodeAlgorithms_hpp

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/NodeContainer.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

namespace implementation {

template <typename Selector, typename Collector>
void select(Node& node, Collector& collector, Selector selector) {

    if (auto f_container = dynamic_cast<NodeContainer*>(&node); f_container) {
        for (auto& child : f_container->children()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_task = dynamic_cast<Task*>(&node); f_task) {
        if (selector(*f_task)) {
            collector(f_task);
        }

        for (auto& child : f_task->aliases()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_alias = dynamic_cast<Alias*>(&node); f_alias) {
        // We don't need to do anything in this case
    }
    else {
        std::ostringstream ss;
        ss << "NodeAlgorithms::select: unexpected node type: " << typeid(node).name();
        throw std::runtime_error(ss.str());
    }
}

template <typename Selector, typename Collector>
void select(const Node& node, Collector& collector, Selector selector) {

    if (auto f_container = dynamic_cast<const NodeContainer*>(&node); f_container) {
        for (auto& child : f_container->children()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_task = dynamic_cast<const Task*>(&node); f_task) {
        if (selector(*f_task)) {
            collector(f_task);
        }

        for (auto& child : f_task->aliases()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_alias = dynamic_cast<const Alias*>(&node); f_alias) {
        // We don't need to do anything in this case
    }
    else {
        std::ostringstream ss;
        ss << "NodeAlgorithms::select: unexpected node type: " << typeid(node).name();
        throw std::runtime_error(ss.str());
    }
}

template <typename Function, typename Collection>
void select(const Defs& defs, Collection& selected, Function selector) {

    for (const auto& s : defs.suites()) {
        select(*s, selected, selector);
    }
}

template <typename NodeX,
          typename Selector,
          typename Collector,
          typename = std::enable_if_t<std::is_base_of<Node, NodeX>::value>>
void select_2(NodeX& node, Collector& collector, Selector selector) {

    if (auto f_container = dynamic_cast<const NodeContainer*>(&node); f_container) {
        for (auto& child : f_container->children()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_task = dynamic_cast<const Task*>(&node); f_task) {
        if (selector(*f_task)) {
            collector(&node);
        }

        for (auto& child : f_task->aliases()) {
            select(*child, collector, selector);
        }
    }
    else if (auto f_alias = dynamic_cast<const Alias*>(&node); f_alias) {
        if (selector(*f_alias)) {
            collector(&node);
        }
    }
    else {
        std::ostringstream ss;
        ss << "NodeAlgorithms::select: unexpected node type: " << typeid(node).name();
        throw std::runtime_error(ss.str());
    }
}

template <typename Function, typename Collection>
void select_2(const Defs& defs, Collection& selected, Function selector) {

    for (const auto& s : defs.suites()) {
        select_2(*s, selected, selector);
    }
}

} // namespace implementation

inline std::vector<Task*> get_all_tasks(const Defs& defs) {
    std::vector<Task*> tasks;

    auto collector = [&tasks](Task* task) { tasks.push_back(task); };
    auto selector  = [](const Node& node) { return node.isTask(); };

    implementation::select(defs, collector, selector);

    return tasks;
}

inline std::vector<Task*> get_all_tasks(Node& node) {
    std::vector<Task*> tasks;

    auto collector = [&tasks](Task* task) { tasks.push_back(task); };
    auto selector  = [](const Node& node) { return node.isTask(); };

    implementation::select(node, collector, selector);

    return tasks;
}

inline std::vector<const Task*> get_all_tasks(const Node& node) {
    std::vector<const Task*> tasks;

    auto collector = [&tasks](const Task* task) { tasks.push_back(task); };
    auto selector  = [](const Node& node) { return node.isTask(); };

    implementation::select(node, collector, selector);

    return tasks;
}

inline std::vector<Node*> get_all_nodes(const Defs& defs) {
    std::vector<Node*> nodes;

    auto collector = [&nodes](Node* node) { nodes.push_back(node); };
    auto selector  = [](const Node& node) { return node.isTask() || node.isAlias(); };

    implementation::select_2(defs, collector, selector);

    return nodes;
}

inline std::vector<Node*> get_all_nodes(Node& node) {
    std::vector<Node*> nodes;

    auto collector = [&nodes](Node* node) { nodes.push_back(node); };
    auto selector  = [](const Node& node) { return node.isTask() || node.isAlias(); };

    implementation::select_2(node, collector, selector);

    return nodes;
}

} // namespace ecf

#endif // ecflow_node_NodeAlgorithms_hpp
