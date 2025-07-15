/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_simulator_FlatAnalyserVisitor_HPP
#define ecflow_simulator_FlatAnalyserVisitor_HPP

#include <sstream>

#include "ecflow/node/NodeTreeVisitor.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"

class Node;

namespace ecf {

class FlatAnalyserVisitor final : public NodeTreeVisitor {
public:
    FlatAnalyserVisitor();
    std::string report() const { return buffer_; }

    bool traverseObjectStructureViaVisitors() const override { return true; }
    void visitDefs(Defs*) override;
    void visitSuite(Suite*) override;
    void visitFamily(Family*) override;
    void visitNodeContainer(NodeContainer*) override;
    void visitTask(Task*) override;

private:
    bool analyse(Node* n);
    std::string buffer_;
    ecf::stringstreambuf ss_{buffer_};
    ecf::Context ctx_ = ecf::Context::make_for(PrintStyle::DEFS);
};

} // namespace ecf

#endif /* ecflow_simulator_FlatAnalyserVisitor_HPP */
