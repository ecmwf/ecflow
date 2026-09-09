/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VNState_HPP
#define ecflow_viewer_VNState_HPP

#include <map>
#include <string>
#include <vector>

#include "VParam.hpp"
#include "ecflow/core/NState.hpp"

class VNode;
class ServerHandler;
class VProperty;

class VNState : public VParam {
public:
    VNState(const std::string& name, NState::State);
    explicit VNState(const std::string& name);

    // Nodes
    static QString toName(const VNode*);
    static QString toDefaultStateName(const VNode*);
    static QString toRealStateName(const VNode*);
    static QColor toColour(const VNode* n);
    static QColor toRealColour(const VNode* n);
    static QColor toFontColour(const VNode* n);
    static QColor toTypeColour(const VNode* n);
    static VNState* toState(const VNode* n);
    static VNState* toDefaultState(const VNode* n);
    static VNState* toRealState(const VNode* n);

    // Server
    static QString toName(ServerHandler*);
    static QColor toColour(ServerHandler*);
    static VNState* toState(ServerHandler*);
    static QColor toFontColour(ServerHandler*);

    static std::vector<VParam*> filterItems();
    static VNState* find(const std::string& name);
    static VNState* find(unsigned char ucId);
    static bool isActive(unsigned char ucId);
    static bool isComplete(unsigned char ucId);
    static bool isSubmitted(unsigned char ucId);

    unsigned char ucId() const { return ucId_; }

    // Called from VConfigLoader
    static void load(VProperty*);

private:
    static std::map<std::string, VNState*> items_;
    unsigned char ucId_{0};
};

#endif /* ecflow_viewer_VNState_HPP */
