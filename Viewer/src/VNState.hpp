//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VNSTATE_HPP_
#define VNSTATE_HPP_

#include <map>
#include <vector>
#include <string>

#include "NState.hpp"
#include "VParam.hpp"

class Node;

class VNState : public VParam
{
public:
	VNState(const std::string& name,NState::State);
	VNState(const std::string& name);

	QColor colour() const;

	static QString toName(Node*);
	static QColor  toColour(Node* n);
	static VNState* toState(Node* n);
	static std::vector<VParam*> filterItems();
	static VNState* find(const std::string& name);
	static void init(const std::string& parFile);

private:
	static std::map<std::string,VNState*> items_;
};

#endif
