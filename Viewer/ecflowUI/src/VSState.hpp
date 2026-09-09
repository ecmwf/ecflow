/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VSState_HPP
#define ecflow_viewer_VSState_HPP

#include <map>
#include <string>
#include <vector>

#include "VParam.hpp"
#include "ecflow/core/SState.hpp"

class ServerHandler;
class VProperty;

class VSState : public VParam {
public:
    VSState(const std::string& name, SState::State);
    explicit VSState(const std::string& name);

    static bool isRunningState(ServerHandler*);
    static QString toName(ServerHandler*);
    static QColor toColour(ServerHandler* n);
    static QColor toFontColour(ServerHandler* n);
    static VSState* toState(ServerHandler* n);
    static VSState* find(const std::string& name);

    // Called from VConfigLoader
    static void load(VProperty* group);

private:
    static std::map<std::string, VSState*> items_;
};

#endif /* ecflow_viewer_VSState_HPP */
