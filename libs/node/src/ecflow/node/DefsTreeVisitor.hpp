/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_DefsTreeVisitor_HPP
#define ecflow_node_DefsTreeVisitor_HPP

#include <string>
#include <vector>

#include <ecflow/node/Alias.hpp>
#include <ecflow/node/Defs.hpp>
#include <ecflow/node/Family.hpp>
#include <ecflow/node/Node.hpp>
#include <ecflow/node/Suite.hpp>
#include <ecflow/node/Task.hpp>

namespace ecf {

struct NodeNotFound : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

template <typename V>
struct DefsTreeVisitor
{
    using path_t  = std::string;
    using nodes_t = std::vector<const Node*>;

    DefsTreeVisitor(const defs_ptr& defs, V& v) : defs_{*defs}, v_{v} {}

    DefsTreeVisitor(const Defs& defs, V& v) : defs_{defs}, v_{v} {}

    void visit_at(const path_t& path) {
        nodes_t nodes = nodes_at(path);
        for (auto node : nodes) {
            visit(*node);
        }
    }

private:
    const Defs& defs_;
    V& v_;

private:
    nodes_t nodes_at(const path_t& path) const {
        nodes_t nodes;
        if (path == "/") {
            std::vector<suite_ptr> suites = defs_.suiteVec();
            std::transform(std::begin(suites), std::end(suites), std::back_inserter(nodes), [](const suite_ptr& ptr) {
                return static_cast<Node*>(ptr.get());
            });
        }
        else {
            nodes = nodes_t{get_node(path).get()};
        }
        return nodes;
    }

    node_ptr get_node(const std::string& path) const {
        if (node_ptr node = defs_.findAbsNode(path); node) {
            return node;
        }

        throw NodeNotFound(std::string{"Unable to find node: "} + path);
    }

    void visit(const Node& node) {

        if (auto found = dynamic_cast<const Suite*>(&node); found) {
            // Visit suite itself
            v_.begin_visit(*found);
            // Visit suite children
            for (auto&& entry : found->nodeVec()) {
                visit(*entry.get());
            }
            v_.end_visit(*found);
            return;
        }

        if (auto found = dynamic_cast<const Family*>(&node); found) {
            // Visit family itself
            v_.begin_visit(*found);
            // Visit family children
            for (auto&& entry : found->nodeVec()) {
                visit(*entry.get());
            }
            v_.end_visit(*found);
            return;
        }

        if (auto found = dynamic_cast<const Task*>(&node); found) {
            // Visit task itself
            v_.begin_visit(*found);
            // Visit task children (i.e. aliases)
            std::vector<alias_ptr> aliases;
            found->get_all_aliases(aliases);
            for (const auto& entry : aliases) {
                visit(*entry.get());
            }
            v_.end_visit(*found);
            return;
        }

        if (auto found = dynamic_cast<const Alias*>(&node); found) {
            // Visit alias itself
            v_.begin_visit(*found);
            v_.end_visit(*found);
            return;
        }

        // Important!
        // If this point is reached, then some kind of Node isn't being properly handled.
        panic();
    }

    static void panic() {
        assert(false);
        std::terminate();
    }
};

} // namespace ecf

#endif /* ecflow_node_DefsTreeVisitor_HPP */
