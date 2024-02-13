/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
