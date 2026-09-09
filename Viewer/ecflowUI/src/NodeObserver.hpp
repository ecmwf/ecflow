/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_NodeObserver_HPP
#define ecflow_viewer_NodeObserver_HPP

#include "ecflow/node/Aspect.hpp"
#include "ecflow/node/Node.hpp"

class VNode;
class VNodeChange;

class NodeObserver {
public:
    NodeObserver()          = default;
    virtual ~NodeObserver() = default;

    virtual void
    notifyBeginNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a, const VNodeChange&)            = 0;
    virtual void notifyEndNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a, const VNodeChange&) = 0;
};

#endif /* ecflow_viewer_NodeObserver_HPP */
