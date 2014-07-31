//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VSTATE_HPP_
#define VSTATE_HPP_

#include <set>
#include <vector>
#include <string>

#include "NState.hpp"
#include "VParam.hpp"

class Node;

class VState : public VParam
{
public:
	VState(QString name,VParam::Type,NState::State);
	VState(QString name,VParam::Type);

	QColor  colour() const;

	static QString toName(Node*);
	static QColor  toColour(Node* n);
	static VParam::Type toType(Node *n);

	static std::vector<VParam*> filterItems();

	static void init();

private:
	static VState* find(Node *n);
	static VState* find(VParam::Type);

	static std::vector<VState*> items_;
};

#endif
