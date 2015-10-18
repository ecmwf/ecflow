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

class VNode;
class ServerHandler;
class VProperty;

class VNState : public VParam
{
public:
	VNState(const std::string& name,NState::State);
	explicit VNState(const std::string& name);

    //Nodes
	static QString toName(const VNode*);
	static QString toDefaultStateName(const VNode*);
    static QString toRealStateName(const VNode*);
	static QColor  toColour(const VNode* n);
	static QColor  toRealColour(const VNode* n);
	static QColor  toFontColour(const VNode* n);
	static VNState* toState(const VNode* n);
	static VNState* toDefaultState(const VNode* n);
	static VNState* toRealState(const VNode* n);

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
