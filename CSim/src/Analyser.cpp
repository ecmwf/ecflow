//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision$ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================

#include "Analyser.hpp"
#include "DefsAnalyserVisitor.hpp"
#include "FlatAnalyserVisitor.hpp"
#include "Defs.hpp"
#include "File.hpp"
#include <iostream>
#include <fstream>

using namespace std;

namespace ecf {

Analyser::Analyser() = default;

void Analyser::run(Defs& theDefs)
{
	// Run flat analysis
	{
		FlatAnalyserVisitor visitor;
		theDefs.acceptVisitTraversor(visitor);

		std::string fileName = "defs.flat";

		std::ofstream file(fileName.c_str());
		file <<  visitor.report();
	}

	// run depth first analysis
	{
		DefsAnalyserVisitor visitor;
		theDefs.acceptVisitTraversor(visitor);

		std::string fileName = "defs.depth";

		std::ofstream file(fileName.c_str(),ios::out);
		file <<  visitor.report();
		file.close();
	}
}

}

