/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Operations_HPP
#define ecflow_node_Operations_HPP

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

namespace ecf {

/**
 * BootstrapDefs, traverses the Node tree when the the server (re)starts or when a new suite is loaded,
 * and is used to bootstrap all required nodes and attributes.
 */
struct BootstrapDefs
{
    inline void operator()(AvisoAttr& attr) const {
        if (attr.parent()->state() == NState::QUEUED) {
            attr.start();
        }
    }
    inline void operator()(MirrorAttr& attr) const { attr.mirror(); }

    template <typename T>
    void operator()(T&& t) const { /* Nothing to do... */ }
};

/**
 * ShutdownDefs, traverses the Node tree when the the server shutsdown/halts,,
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

namespace detail {

template <typename V, typename I>
void visit_all(const std::vector<std::shared_ptr<I>>& all, V&& visitor) {
    for (auto& item : all) {
        visit(*item, std::forward<V>(visitor));
    }
}

template <typename V, typename I>
void visit_attrs(std::vector<I>& all, V&& visitor) {
    for (auto& i : all) {
        visit(i, std::forward<V>(visitor));
    }
}

template <typename I>
struct Visitor
{
    template <typename V>
    void operator()(V&& v) {
        v(item_);
    }

    I& item_;
};

template <>
struct Visitor<Task>
{
    template <typename V>
    void operator()(V&& v) {
        v(task_);
        visit_attrs(task_.avisos(), std::forward<V>(v));
        visit_attrs(task_.mirrors(), std::forward<V>(v));
    }

    Task& task_;
};

template <>
struct Visitor<Family>
{
    template <typename V>
    void operator()(V&& v) {
        v(family_);
        visit_all(family_.children(), std::forward<V>(v));
    }

    Family& family_;
};

template <>
struct Visitor<Node>
{
    template <typename V>
    void operator()(V&& v) {
        if (auto* family_ptr = dynamic_cast<Family*>(&node_)) {
            visit(*family_ptr, std::forward<V>(v));
        }
        else if (auto* task_ptr = dynamic_cast<Task*>(&node_)) {
            visit(*task_ptr, std::forward<V>(v));
        }
        if (auto* alias_ptr = dynamic_cast<Alias*>(&node_)) {
            visit(*alias_ptr, std::forward<V>(v));
        }
    }

    Node& node_;
};

template <>
struct Visitor<Suite>
{
    template <typename V>
    void operator()(V&& v) {
        v(*this);
        visit_all(suite_.children(), std::forward<V>(v));
    }

    Suite& suite_;
};

template <>
struct Visitor<Defs>
{
    template <typename V>
    void operator()(V&& v) {
        v(*this);

        visit_all(defs_.suiteVec(), std::forward<V>(v));
    }

    Defs& defs_;
};

} // namespace detail

template <typename V, typename I>
void visit(I& item, V&& visitor) {
    detail::Visitor<I>{item}(std::forward<V>(visitor));
}

} // namespace ecf

#endif // ecflow_node_Operations_HPP
