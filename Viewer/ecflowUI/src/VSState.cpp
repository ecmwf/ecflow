/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VSState.hpp"

#include <cstdlib>
#include <map>
#include <unistd.h>

#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <sys/stat.h>
#include <sys/types.h>

#include "ConnectState.hpp"
#include "ServerHandler.hpp"
#include "UserMessage.hpp"
#include "VConfigLoader.hpp"
#include "VProperty.hpp"
#include "ecflow/node/Submittable.hpp"

std::map<std::string, VSState*> VSState::items_;
static std::map<SState::State, VSState*> stateMap_;

static VSState haltedSt("halted", SState::HALTED);
static VSState runningSt("running", SState::RUNNING);
static VSState shutSt("shutdown", SState::SHUTDOWN);
static VSState disconnectedSt("disconnected");

VSState::VSState(const std::string& name, SState::State Sstate) : VParam(name) {
    items_[name]      = this;
    stateMap_[Sstate] = this;
}

VSState::VSState(const std::string& name) : VParam(name) {
    items_[name] = this;
}

//===============================================================
//
// Static methods
//
//===============================================================

bool VSState::isRunningState(ServerHandler* s) {
    return toState(s) == items_["running"];
}

VSState* VSState::toState(ServerHandler* s) {
    if (!s) {
        return nullptr;
    }

    if (s->connectState()->state() != ConnectState::Normal) {
        return items_["disconnected"];
    }

    auto it = stateMap_.find(s->serverState());
    if (it != stateMap_.end()) {
        return it->second;
    }

    return nullptr;
}

VSState* VSState::find(const std::string& name) {
    auto it = items_.find(name);
    if (it != items_.end()) {
        return it->second;
    }

    return nullptr;
}

//
// Has to be very quick!!
//

QColor VSState::toColour(ServerHandler* s) {
    VSState* obj = VSState::toState(s);
    return (obj) ? (obj->colour()) : QColor();
}

QColor VSState::toFontColour(ServerHandler* s) {
    VSState* obj = VSState::toState(s);
    return (obj) ? (obj->fontColour()) : QColor();
}

QString VSState::toName(ServerHandler* s) {
    VSState* obj = VSState::toState(s);
    return (obj) ? (obj->name()) : QString();
}

void VSState::load(VProperty* group) {
    Q_FOREACH (VProperty* p, group->children()) {
        if (VSState* obj = VSState::find(p->strName())) {
            obj->setProperty(p);
        }
    }
}

static SimpleLoader<VSState> loader("sstate");
