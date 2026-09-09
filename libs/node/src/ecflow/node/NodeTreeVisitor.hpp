/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_NodeTreeVisitor_HPP
#define ecflow_node_NodeTreeVisitor_HPP

class Defs;
class Suite;
class Family;
class Task;
class NodeContainer;

namespace ecf {

class NodeTreeVisitor {
public:
    virtual ~NodeTreeVisitor();

    virtual bool traverseObjectStructureViaVisitors() const { return false; }
    virtual void visitDefs(Defs*)                   = 0;
    virtual void visitSuite(Suite*)                 = 0;
    virtual void visitFamily(Family*)               = 0;
    virtual void visitNodeContainer(NodeContainer*) = 0;
    virtual void visitTask(Task*)                   = 0;
};

} // namespace ecf

#endif /* ecflow_node_NodeTreeVisitor_HPP */
