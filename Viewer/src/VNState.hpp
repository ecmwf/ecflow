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
class ServerHandler;
class VProperty;

class VNState : public VParam
{
public:
	VNState(const std::string& name,NState::State);
	VNState(const std::string& name);

    //Nodes
	static QString toName(Node*);
	static QString toDefaultStateName(Node*);
	static QColor  toColour(Node* n);
	static QColor  toFontColour(Node* n);
	static VNState* toState(Node* n);
	static VNState* toDefaultState(Node* n);

	//Server
	static QString toName(ServerHandler*);
	static QColor  toColour(ServerHandler*);
	static VNState* toState(ServerHandler*);
	static QColor  toFontColour(ServerHandler*);

	static std::vector<VParam*> filterItems();
	static VNState* find(const std::string& name);
	
    //Called from VConfigLoader
    static void load(VProperty*);

private:
	static std::map<std::string,VNState*> items_;
};

#endif
