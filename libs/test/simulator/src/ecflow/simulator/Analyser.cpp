/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/simulator/Analyser.hpp"

#include <fstream>

#include "ecflow/node/Defs.hpp"
#include "ecflow/simulator/DefsAnalyserVisitor.hpp"
#include "ecflow/simulator/FlatAnalyserVisitor.hpp"

namespace ecf {

Analyser::Analyser() = default;

void Analyser::run(Defs& theDefs) {
    // Run flat analysis
    {
        FlatAnalyserVisitor visitor;
        theDefs.acceptVisitTraversor(visitor);

        std::string fileName = "defs.flat";

        std::ofstream file(fileName.c_str());
        if (!file.is_open()) {
            throw std::runtime_error("Analyser::run: Failed to open file \"" + fileName + "\"");
        }

        file << visitor.report();
    }

    // run depth first analysis
    {
        DefsAnalyserVisitor visitor;
        theDefs.acceptVisitTraversor(visitor);

        std::string fileName = "defs.depth";

        std::ofstream file(fileName.c_str(), std::ios::out);
        if (!file.is_open()) {
            throw std::runtime_error("Analyser::run: Failed to open file \"" + fileName + "\"");
        }

        file << visitor.report();
        file.close();
    }
}

} // namespace ecf
