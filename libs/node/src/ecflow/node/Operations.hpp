// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

/**
 * BootstrapDefs, traverses the Node tree when the server (re)starts or when a new suite is loaded,
 * and is used to bootstrap all required nodes and attributes.
 */
struct BootstrapDefs
{
    inline void operator()(AvisoAttr& attr) const {
        if (attr.parent() && attr.parent()->state() == NState::QUEUED) {
            attr.start();
        }
    }
    inline void operator()(MirrorAttr& attr) const { attr.mirror(); }

    template <typename T>
    void operator()(T&& t) const { /* Nothing to do... */ }
};

/**
 * ShutdownDefs, traverses the Node tree when the server shuts down/halts,
 * and is used to shutdown all required nodes and attributes.
 */
struct ShutdownDefs
{
    inline void operator()(AvisoAttr& attr) const { attr.finish(); }
    inline void operator()(MirrorAttr& attr) const { attr.finish(); }

    template <typename T>
    void operator()(T&& t) const { /* Nothing to do... */ }
};

/**
 * ActivateAll, traverses the Node tree periodicablly, and effectively triggers the synchronization between the
 * main and background threads.
 */
struct ActivateAll
{
    inline void operator()(MirrorAttr& attr) const { attr.mirror(); }

    template <typename T>
    void operator()(T&& t) const { /* Nothing to do... */ }
};

template <typename V, typename I>
void visit_all(I& item, V&& visitor);

namespace detail {

// n.b. The recursive calls below are qualified, so that overload resolution never picks the generic ecf::visit_all
//      for a vector of nodes (which would apply the visitor to the vector itself, and stop the traversal)

template <typename V, typename I>
void visit_each(const std::vector<std::shared_ptr<I>>& all, V&& visitor) {
    for (auto& item : all) {
        ecf::visit_all(*item, visitor);
    }
}

template <typename V, typename I>
void visit_attrs(std::vector<I>& all, V&& visitor) {
    for (auto& i : all) {
        ecf::visit_all(i, visitor);
    }
}

template <typename I>
struct VisitorAll
{
    template <typename V>
    void operator()(V&& v) {
        v(item_);
    }

    I& item_;
};

template <>
struct VisitorAll<Task>
{
    template <typename V>
    void operator()(V&& v) {
        v(task_);
        detail::visit_attrs(task_.avisos(), v);
        detail::visit_attrs(task_.mirrors(), v);
    }

    Task& task_;
};

template <>
struct VisitorAll<Family>
{
    template <typename V>
    void operator()(V&& v) {
        v(family_);
        detail::visit_each(family_.children(), v);
    }

    Family& family_;
};

template <>
struct VisitorAll<Node>
{
    template <typename V>
    void operator()(V&& v) {
        if (auto* family_ptr = dynamic_cast<Family*>(&node_)) {
            ecf::visit_all(*family_ptr, v);
        }
        else if (auto* task_ptr = dynamic_cast<Task*>(&node_)) {
            ecf::visit_all(*task_ptr, v);
        }
        if (auto* alias_ptr = dynamic_cast<Alias*>(&node_)) {
            ecf::visit_all(*alias_ptr, v);
        }
    }

    Node& node_;
};

template <>
struct VisitorAll<Suite>
{
    template <typename V>
    void operator()(V&& v) {
        v(suite_);
        detail::visit_each(suite_.children(), v);
    }

    Suite& suite_;
};

template <>
struct VisitorAll<Defs>
{
    template <typename V>
    void operator()(V&& v) {
        v(defs_);
        detail::visit_each(defs_.suites(), v);
    }

    Defs& defs_;
};

template <typename I>
struct VisitorParent
{
    template <typename V>
    void operator()(V&& v) {
        v(item_);
        if (auto* parent = item_.parent(); parent) {
            visit_parents(*parent, std::forward<V>(v));
        }
    }

    I& item_;
};

} // namespace detail

/**
 * Traverses all nodes downwards in the tree, including the given item and all its children.
 *
 * @tparam V - Visitor type
 * @tparam I - Item type
 * @param item - The 'root' item to traverse
 * @param visitor - The visitor to apply to each item
 */
template <typename V, typename I>
void visit_all(I& item, V&& visitor) {
    detail::VisitorAll<I>{item}(std::forward<V>(visitor));
}

/**
 * Traverses nodes upwards in the tree, including the given item and all its parents.
 *
 * @tparam V - Visitor type
 * @tparam I - Item type
 * @param item - The 'leaf' item to traverse
 * @param visitor - The visitor to apply to each item
 */
template <typename V, typename I>
void visit_parents(I& item, V&& visitor) {
    detail::VisitorParent<I>{item}(std::forward<V>(visitor));
}

} // namespace ecf
