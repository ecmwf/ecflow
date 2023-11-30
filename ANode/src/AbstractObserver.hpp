/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_AbstractObserver_HPP
#define ecflow_core_AbstractObserver_HPP

#include <vector>

#include "Aspect.hpp"

class Node;
class Defs;

class AbstractObserver {
public:
    virtual ~AbstractObserver()                                                   = default;

    virtual void update_start(const Node*, const std::vector<ecf::Aspect::Type>&) = 0;
    virtual void update_start(const Defs*, const std::vector<ecf::Aspect::Type>&) = 0;

    virtual void update(const Node*, const std::vector<ecf::Aspect::Type>&)       = 0;
    virtual void update(const Defs*, const std::vector<ecf::Aspect::Type>&)       = 0;

    /// After this call, the node will be deleted, hence observers must *NOT* use the pointers
    virtual void update_delete(const Node*) {}
    virtual void update_delete(const Defs*) {}
};

#endif /* ecflow_core_AbstractObserver_HPP */
