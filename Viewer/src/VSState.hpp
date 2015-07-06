//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VSSTATE_HPP_
#define VSSTATE_HPP_

#include <map>
#include <vector>
#include <string>

#include "SState.hpp"
#include "VParam.hpp"

class ServerHandler;
class VProperty;

class VSState : public VParam
{
public:
	VSState(const std::string& name,SState::State);
	explicit VSState(const std::string& name);

    static bool isRunningState(ServerHandler*);
	static QString toName(ServerHandler*);
	static QColor  toColour(ServerHandler* n);
	static QColor  toFontColour(ServerHandler* n);
	static VSState* toState(ServerHandler* n);
	static VSState* find(const std::string& name);
    
    //Called from VConfigLoader
    static void load(VProperty* group);

private:
	static std::map<std::string,VSState*> items_;
};

#endif
