/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_core_AbstractObserver_HPP
#define ecflow_core_AbstractObserver_HPP

#include <vector>

#include "ecflow/node/Aspect.hpp"

class Node;
class Defs;

class AbstractObserver {
public:
    virtual ~AbstractObserver() = default;

    virtual void update_start(const Node*, const std::vector<ecf::Aspect::Type>&) = 0;
    virtual void update_start(const Defs*, const std::vector<ecf::Aspect::Type>&) = 0;

    virtual void update(const Node*, const std::vector<ecf::Aspect::Type>&) = 0;
    virtual void update(const Defs*, const std::vector<ecf::Aspect::Type>&) = 0;

    /// After this call, the node will be deleted, hence observers must *NOT* use the pointers
    virtual void update_delete(const Node*) {}
    virtual void update_delete(const Defs*) {}
};

#endif /* ecflow_core_AbstractObserver_HPP */
